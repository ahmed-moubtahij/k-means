#include <concepts>
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <ranges>
#include <fmt/ranges.h>

#define FWD(x) static_cast<decltype(x)&&>(x)

//dispatch: Akin to a std::partition_copy albeit with output to multiple ranges
//src_range should be const& but ranges::views don't support const begin/end
template<std::ranges::input_range R1,
         std::ranges::input_range R2,
         typename F,
         typename Comp = std::ranges::less,
         typename Proj1 = std::identity,
         typename Proj2 = std::identity>
constexpr void dispatch(R1&& src_range,
                        R2&& buckets, F dispatcher,
                        Comp threeway_comp = {},
                        Proj1 proj_bucket = {}, Proj2 proj_comp = {})
{
    for(auto&& e: FWD(src_range)){
        auto& target_bucket = *dispatcher(FWD(buckets), threeway_comp(e), proj_comp);
        std::invoke(proj_bucket, target_bucket).push_back(e);
    }
}

namespace kmn{

namespace rn = std::ranges;
namespace rnv = rn::views;

void print_clusters(auto const& clusters, std::size_t k)
{
    for(auto const& [centroid, satellites]: clusters | rnv::take(k)){
        print("centroid: {}\nsatellites: {}\n\n",
               centroid, satellites);
    }
}

template<typename T>
concept arithmetic = std::integral<T> or std::floating_point<T>;
//Class template DataPoint
template<arithmetic T, std::size_t D>
struct DataPoint final : private std::array<T, D> {
    using typename std::array<T, D>::value_type;
    using std::array<T, D>::begin;
    using std::array<T, D>::end;
    using std::array<T, D>::cbegin;
    using std::array<T, D>::cend;
    using std::array<T, D>::operator[];
    using std::array<T, D>::size;

    constexpr DataPoint() noexcept = default;
    constexpr DataPoint(DataPoint const&) noexcept = default;
    constexpr DataPoint& operator=(DataPoint const&) noexcept = default;
    constexpr DataPoint(std::convertible_to<value_type> auto... coords) noexcept
        requires (sizeof...(coords) == D)
        : std::array<T, D>{coords...} {}
    
    friend constexpr bool operator<=>(DataPoint const&, DataPoint const&) = default;
    
    constexpr auto operator+(DataPoint const& rhs) const
    {
        DataPoint res;
        rn::transform(rhs, *this, res.begin(), std::plus{});
        return res;
    }

    //operator/ overload for floating point value type; result's value type matches it
    constexpr auto operator/(arithmetic auto n) const
        requires (std::floating_point<value_type>)
    {
        DataPoint<value_type, D> res;
        rn::transform(*this, res.begin(),
        [&n](value_type e){ return e / static_cast<value_type>(n); });
        return res;
    }

    //operator/ overload for integer T => result's value type = double
    constexpr auto operator/(arithmetic auto n) const
    {
        DataPoint<double, D> res;
        rn::transform(*this, res.begin(),
        [&n](value_type e){ return e / static_cast<double>(n); });
                      
        return res;
    }

    template<std::floating_point U>
    constexpr explicit operator DataPoint<U, D>() const
    {
        DataPoint<double, D> res;
        rn::transform(*this, res.begin(),
        [](value_type num){ return static_cast<U>(num); });
                      
        return res;
    }
};
//DataPoint deduction guide
template<arithmetic T, typename... Us>
DataPoint(T, Us...) -> DataPoint<T, sizeof...(Us) + 1>;

namespace hlpr{
    template<typename T, std::size_t D>
    struct select_centroid
    {
        //A DataPoint's value type T is constrained to be arithmetic
        //So T here is an integral if the floating_point overload is discarded
        using type = DataPoint<double, D>;
    };

    template<std::floating_point T, std::size_t D>
    struct select_centroid<T, D>
    {
        using type = DataPoint<T, D>;
    };

    template<typename T, std::size_t D>
    using select_centroid_t = typename select_centroid<T, D>::type;
}
//Aggregate template Cluster
template<typename T, std::size_t D>
struct Cluster{
    using data_point_t = DataPoint<T, D>;
    using centroid_t = hlpr::select_centroid_t<T, D>; 
    using satellites_t = std::vector<data_point_t>; 
    centroid_t centroid;    
    satellites_t satellites;
};

//sqr_dist: computes euclidean square distance between two data points
template<typename T1, typename T2, std::size_t D>
auto sqr_distance(DataPoint<T1,D> const& dp1,
                  DataPoint<T2,D> const& dp2)
{
    return std::transform_reduce(
           dp1.cbegin(), dp1.cend(), dp2.cbegin(), 0,
           [](T1 a, T2 b){ return a + b; },
           [](T1 a, T2 b){ return (a - b)*(a - b); });
};
//distance_from: Function Object Comparator
//               of distances from two points to a reference point
template<typename T, std::size_t D>
struct distance_from{
    using ref_point_t = DataPoint<T, D>;
    ref_point_t m_pt;
    
    distance_from() = delete;
    distance_from& operator=(distance_from const&) = delete;
    constexpr distance_from(ref_point_t const& pt) : m_pt{pt} {}
    
    template<typename U>
    constexpr bool operator()(DataPoint<U, D> const& c1,
                              DataPoint<U, D> const& c2) const
    {
        auto const dist_c1_pt = sqr_distance(c1, m_pt);
        auto const dist_c2_pt = sqr_distance(c2, m_pt);
        if (dist_c1_pt == 0) return false; //if m_pt is c1, then c1 < c2 is false
        if (dist_c2_pt == 0) return true;  //if m_pt is c2, then c1 < c2 is true
        return dist_c1_pt < dist_c2_pt;
    }
};

template<typename T, std::size_t D>
void init_centroids(auto&& out_clusters, auto const& data_points)
{
    using cluster_t = Cluster<T, D>;    
    auto centroids = FWD(out_clusters) | rnv::transform(&cluster_t::centroid);
    auto const k = rn::distance(out_clusters);
    //Initialize centroids with a sample of K points from data_points
    if constexpr(std::floating_point<T>){
        rn::sample(data_points, rn::begin(centroids),
                   k, std::mt19937{std::random_device{}()});
    } else { //else, data_points is a range of DataPoints of integral value types T
        //T needs to be a floating point type to match centroids' T
        //because centroids get updated with data_points' means
        //and a mean's result being intrinsically a floating point type
        rn::sample(data_points | rnv::transform(
                                [](DataPoint<T, D> const& pt)
                                { return static_cast<DataPoint<double, D>>(pt); }),
                   rn::begin(centroids), k,
                   std::mt19937{std::random_device{}()});
    }
}

template<typename T, std::size_t D>
void update_satellites(auto&& clusters,
                       auto&& non_centroid_data)
{
    using cluster_t = Cluster<T, D>;
    
    rn::for_each(clusters,
                //explicitizing auto here to cluster_t::satellites_t doesn't compile on gcc 10.2                
                 [](auto&& sats){ FWD(sats).clear(); },
                 &cluster_t::satellites);
    
    auto constexpr find_closest_centroid =
    [](auto&& clusters, auto comp, auto proj)
    { return rn::min_element(FWD(clusters), comp, proj); };

    auto constexpr comp_dist_to_centroid =
    [](DataPoint<T, D> const& pt){ return distance_from{ pt }; };

    //Dispatch every data point to the cluster whose centroid is closest
    dispatch(FWD(non_centroid_data),
             FWD(clusters), 
             find_closest_centroid, comp_dist_to_centroid,
             &cluster_t::satellites, &cluster_t::centroid);
    
}

template<typename T, std::size_t D>
void update_centroids(auto&& out_clusters)
{
    using cluster_t = Cluster<T, D>;
    
    auto constexpr mean =
    [](cluster_t::satellites_t const& sats)
    { return std::reduce(std::cbegin(sats), std::cend(sats)) / std::size(sats); };
    
    auto constexpr has_satellites = [](cluster_t const& cluster)
    { return not cluster.satellites.empty(); };
    
    auto&& centroids = FWD(out_clusters) | rnv::transform(&cluster_t::centroid);
    //Update every centroid with its satellites mean
    rn::transform(FWD(out_clusters) | rnv::filter(has_satellites),
                  rn::begin(centroids),
                  mean, &cluster_t::satellites);
}

template<arithmetic PT_VALUE_T, std::size_t D>
constexpr void k_means_impl(auto const& data_points, auto&& out_clusters,
                            std::size_t k, std::size_t n)
{
    if (rn::distance(data_points) < k or k < 2) return;

    auto&& k_out_clusters = FWD(out_clusters) | rnv::take(k);
    
    init_centroids<PT_VALUE_T, D>(k_out_clusters, data_points);
        
    update_satellites<PT_VALUE_T, D>(k_out_clusters, data_points);

    while(n--)
    {
        update_centroids<PT_VALUE_T, D>(k_out_clusters);
        update_satellites<PT_VALUE_T, D>(k_out_clusters, data_points);
    }
}

namespace hlpr{
    template<typename T>
    struct is_data_point : std::false_type {};
    template<typename T, std::size_t D>
    struct is_data_point<DataPoint<T, D>> : std::true_type {};
    
    template<typename T>
    struct data_point_size;
    template<typename T, std::size_t D>
    struct data_point_size<DataPoint<T, D>> : std::integral_constant<std::size_t, D> {};

    template<typename T>
    struct is_cluster : std::false_type {};
    template<typename T, std::size_t D>
    struct is_cluster<Cluster<T, D>> : std::true_type{};
}
template<typename R>
concept data_points_range = hlpr::is_data_point<rn::range_value_t<R>>::value;

template<typename R>
concept clusters_out_range = rn::output_range< R, rn::range_value_t< R > >
and rn::forward_range< R >
and hlpr::is_cluster< rn::range_value_t< R > >::value;
         
constexpr void
k_means(data_points_range auto const& data_points,
        clusters_out_range auto&& out_range, 
        std::size_t k, std::size_t n)
    //Ensure that the output clusters' data point types are the same (in value type and size)
    //as the input range's data point types
    requires (std::is_same_v<rn::range_value_t<decltype(data_points)>,
                             typename rn::range_value_t<decltype(out_range)>::data_point_t>)
{
    using data_point_t = rn::range_value_t<decltype(data_points)>;
    using point_value_t = data_point_t::value_type;
    auto constexpr dimension = hlpr::data_point_size<data_point_t>::value;

    k_means_impl<point_value_t, dimension>(FWD(data_points), FWD(out_range), k, n);
}

} // namespace kmn

int main(){
    using std::array, std::vector, kmn::DataPoint;
    using kmn::print_clusters, kmn::k_means;
    
    //INPUT ranges
    auto const int_arr_df =
    array{DataPoint(1, 2, 3), DataPoint(4, 5, 6), DataPoint(7, 8, 9),
          DataPoint(10, 11, 12), DataPoint(13, 14, 15), DataPoint(16, 17, 18),
          DataPoint(19, 20, 21), DataPoint(22, 23, 24), DataPoint(25, 26, 27),
          DataPoint(28, 29, 30), DataPoint(31, 32, 33), DataPoint(34, 35, 36),
          DataPoint(37, 38, 39), DataPoint(40, 41, 42), DataPoint(43, 44, 45)};
    
    auto const double_arr_df =
    array{DataPoint(1., 2., 3.),    DataPoint(4., 5., 6.),    DataPoint(7., 8., 9.),
          DataPoint(10., 11., 12.), DataPoint(13., 14., 15.), DataPoint(16., 17., 18.),
          DataPoint(19., 20., 21.), DataPoint(22., 23., 24.), DataPoint(25., 26., 27.),
          DataPoint(28., 29., 30.), DataPoint(31., 32., 33.), DataPoint(34., 35., 36.),
          DataPoint(37., 38., 39.), DataPoint(40., 41., 42.), DataPoint(43., 44., 45.)};

    auto const float_arr_df =
    array{DataPoint(1.f, 2.f, 3.f),    DataPoint(4.f, 5.f, 6.f),    DataPoint(7.f, 8.f, 9.f),
          DataPoint(10.f, 11.f, 12.f), DataPoint(13.f, 14.f, 15.f), DataPoint(16.f, 17.f, 18.f),
          DataPoint(19.f, 20.f, 21.f), DataPoint(22.f, 23.f, 24.f), DataPoint(25.f, 26.f, 27.f),
          DataPoint(28.f, 29.f, 30.f), DataPoint(31.f, 32.f, 33.f), DataPoint(34.f, 35.f, 36.f),
          DataPoint(37.f, 38.f, 39.f), DataPoint(40.f, 41.f, 42.f), DataPoint(43.f, 44.f, 45.f)};

    auto const int_vec_df =
    vector{DataPoint(1, 2, 3),   DataPoint(4, 5, 6),    DataPoint(7, 8, 9),
          DataPoint(10, 11, 12), DataPoint(13, 14, 15), DataPoint(16, 17, 18),
          DataPoint(19, 20, 21), DataPoint(22, 23, 24), DataPoint(25, 26, 27),
          DataPoint(28, 29, 30), DataPoint(31, 32, 33), DataPoint(34, 35, 36),
          DataPoint(37, 38, 39), DataPoint(40, 41, 42), DataPoint(43, 44, 45)};
              
    //OUTPUT range
    std::array<kmn::Cluster<int, 3>, 6> clusters;
    
    //CALL
    std::size_t const k{ 4 }, n{ 10 };
    k_means(int_arr_df, clusters, k, n);
    
    //DISPLAY
    print("OUTPUT clusters:\n\n");
    print_clusters(clusters, k);

    return 0;
}
