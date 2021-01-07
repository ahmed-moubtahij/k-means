#include <concepts>
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <ranges>
#include <fmt/ranges.h>

#define FWD(x) static_cast<decltype(x)&&>(x)

//TODO: the std::array overload should be a std::span (also readup on diff with contiguous_range)

//sarah notes:
// instead of using the range member functions, you should be using the std::ranges::* function objects
// for instance rn::begin(*) instead of *.begin()

// i also wouldn't bother with using cbegin and cend
// they provide a false sense of security
// since views have shallow const semantics

//TODO: marcorubini's function object wrapper implementation
//TODO: Lesley Lai's non-generic lambdas

using fmt::print;

template<typename ...Ts>
[[deprecated]] constexpr bool print_type = true;

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

void print_clusters(auto const& clusters)
{
    for(auto const& [centroid, satellites]: clusters){
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

    constexpr auto operator/(arithmetic auto n) const
    {
        DataPoint<double, D> res;
        rn::transform(*this, res.begin(),
                      [&n](auto const& e)
                      { return e / static_cast<double>(n); });
        return res;
    }

    constexpr explicit operator DataPoint<double, D>() const
    {
        DataPoint<double, D> res;
        rn::transform(*this, res.begin(),
                      [](auto const& num)
                      { return static_cast<double>(num); });
        return res;
    }
};
//DataPoint deduction guide
template<arithmetic T, typename... Us>
DataPoint(T, Us...) -> DataPoint<T, sizeof...(Us) + 1>;

//Aggregate template Cluster
template<typename T, std::size_t D>
struct Cluster{
    using centroid_t = DataPoint<double, D>;
    using satellites_t = std::vector<DataPoint<T, D>>; 
    
    centroid_t centroid;    
    satellites_t satellites;
};

//sqr_dist: computes square distance between two data points
template<typename T1, std::size_t D, typename T2>
auto sqr_distance(DataPoint<T1,D> const& dp1,
                  DataPoint<T2,D> const& dp2)
{
    return
    std::transform_reduce(dp1.cbegin(), dp1.cend(),
                          dp2.cbegin(), 0,
                          [](auto a, auto b){ return a+b; },
                          [](auto a, auto b){ return (a-b)*(a-b); }
                         );
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

template<typename T, std::size_t D, std::size_t K>
auto init_centroids(auto&& clusters, auto const& data_points)
{
    using cluster_t = Cluster<T, D>;    
    auto centroids = FWD(clusters) | rnv::transform(&cluster_t::centroid);
    //Initialize centroids with a sample of K points from data_points
    if constexpr(std::floating_point<T>){
        rn::sample(data_points, centroids.begin(),
                   K, std::mt19937{std::random_device{}()});
    } else { //else, data_points is a range of DataPoints of integral value types T
        //T needs to be a floating point type to match centroids' T,
        //that is because centroids get updated with data_points' means
        rn::sample(data_points |
                   rnv::transform([](auto const& pt)
                   { return static_cast<DataPoint<double, D>>(pt); }),
                   centroids.begin(),
                   K, std::mt19937{std::random_device{}()});
    }
    return centroids;
}

template<typename T, std::size_t D>
void update_satellites(auto&& clusters,
                       auto&& non_centroid_data)
{
    using cluster_t = Cluster<T, D>;
    
    rn::for_each(clusters,
                 [](auto&& satellites){ FWD(satellites).clear();},
                 &cluster_t::satellites);
    
    auto constexpr find_closest_centroid =
    [](auto&& clusters, auto comp, auto proj)
    { return rn::min_element(FWD(clusters), comp, proj); };

    auto constexpr comp_dist_to_centroid =
    [](auto const& data_pt){ return distance_from{data_pt}; };

    //Dispatch every data point to the cluster whose centroid is closest
    dispatch(FWD(non_centroid_data),
             FWD(clusters), 
             find_closest_centroid, comp_dist_to_centroid,
             &cluster_t::satellites, &cluster_t::centroid);
    
}

template<typename T, std::size_t D>
void update_centroids(auto&& clusters)
{
    using cluster_t = Cluster<T, D>;
    
    auto constexpr mean = [](auto const& r)
    { return std::reduce(r.cbegin(), r.cend()) / r.size(); };
    
    auto constexpr has_satellites = [](auto const& cluster)
    { return not cluster.satellites.empty(); };
    
    auto&& centroids = FWD(clusters) | rnv::transform(&cluster_t::centroid);
    //Update every centroid with its satellites mean
    rn::transform(FWD(clusters) | rnv::filter(has_satellites),
                  centroids.begin(),
                  mean, &cluster_t::satellites);
}

//k_means_impl: ARRAY overload
template<std::size_t K, arithmetic PT_VALUE_T,
         std::size_t D, std::size_t SZ>
constexpr auto k_means_ovrl(std::array<DataPoint<PT_VALUE_T, D>, SZ> const& data_points, std::size_t n)
-> std::array<Cluster<PT_VALUE_T, D>, K>
{
    fmt::print("CALLING STD::ARRAY OVERLOAD\n\n");
    if (SZ < K or K < 2) return {{}};

    using cluster_t = Cluster<PT_VALUE_T, D>;
    std::array<cluster_t, K> clusters;

    auto centroids = init_centroids<PT_VALUE_T, D, K>(clusters, data_points);
        
    auto const is_not_centroid = [&centroids](auto const& pt)
    { return rn::find(centroids, static_cast<DataPoint<double, D>>(pt)) == centroids.end(); };
   
    update_satellites<PT_VALUE_T, D>(clusters, data_points | rnv::filter(is_not_centroid));

    while(n--){
        update_centroids<PT_VALUE_T, D>(clusters);
        update_satellites<PT_VALUE_T, D>(clusters, data_points | rnv::filter(is_not_centroid));
    }
    
    return clusters;
}

//k_means_impl: RANGE overload
template<std::size_t K, arithmetic PT_VALUE_T, std::size_t D>
constexpr auto k_means_ovrl(auto const& data_points, std::size_t n)
//TODO: Trailing return type
{
    fmt::print("CALLING RANGES OVERLOAD\n\n");
    // if (std::ranges::distance(data_points) < K or K < 2) return {{}};
    
    //TODO: This shouldn't be a std::array but the data_points' decltype
    std::array<Cluster<PT_VALUE_T, D>, K> clusters;

    auto centroids = init_centroids<PT_VALUE_T, D, K>(clusters, data_points);
        
    auto const is_not_centroid = [&centroids/*, &D*/](auto const& pt)
    { return rn::find(centroids, static_cast<DataPoint<double, D>>(pt)) == centroids.end(); };
   
    update_satellites<PT_VALUE_T, D>(clusters, data_points | rnv::filter(is_not_centroid));

    while(n--){
        update_centroids<PT_VALUE_T, D>(clusters);
        update_satellites<PT_VALUE_T, D>(clusters, data_points | rnv::filter(is_not_centroid));
    }
    
    return clusters;
}

template<typename T>
struct is_data_point : std::false_type {};
template<typename T, std::size_t D>
struct is_data_point<DataPoint<T, D>> : std::true_type {};
template<typename R>
concept data_points_range = is_data_point<std::ranges::range_value_t<R>>::value;

template<typename T>
struct data_point_size;
template<typename T, std::size_t D>
struct data_point_size<DataPoint<T, D>> : std::integral_constant<std::size_t, D> {};

template<std::size_t K>
constexpr auto k_means(data_points_range auto const& data_points, std::size_t n)
{
    using data_point_t = rn::range_value_t<decltype(data_points)>;
    using point_value_t = data_point_t::value_type;
    auto constexpr dimension = data_point_size<data_point_t>::value;

    return k_means_ovrl<K, point_value_t, dimension>(FWD(data_points), n);
}

} // namespace kmn

int main(){
    using std::array, std::vector, kmn::DataPoint;
    using kmn::print_clusters, kmn::k_means;

    auto const int_arr_df = array{DataPoint(1, 2, 3),
                                  DataPoint(4, 5, 6),
                                  DataPoint(7, 8, 9),
                                  DataPoint(10, 11, 12),
                                  DataPoint(13, 14, 15),
                                  DataPoint(16, 17, 18),
                                  DataPoint(19, 20, 21),
                                  DataPoint(22, 23, 24),
                                  DataPoint(25, 26, 27),
                                  DataPoint(28, 29, 30),
                                  DataPoint(31, 32, 33),
                                  DataPoint(34, 35, 36),
                                  DataPoint(37, 38, 39),
                                  DataPoint(40, 41, 42)};
    auto const double_arr_df = array{DataPoint(1.0, 2.0, 3.0),
                                     DataPoint(4.0, 5.0, 6.0),
                                     DataPoint(7.0, 8.0, 9.0),
                                     DataPoint(10.0, 11.0, 12.0),
                                     DataPoint(13.0, 14.0, 15.0),
                                     DataPoint(16.0, 17.0, 18.0),
                                     DataPoint(19.0, 20.0, 21.0),
                                     DataPoint(22.0, 23.0, 24.0),
                                     DataPoint(25.0, 26.0, 27.0),
                                     DataPoint(28.0, 29.0, 30.0),
                                     DataPoint(31.0, 32.0, 33.0),
                                     DataPoint(34.0, 35.0, 36.0),
                                     DataPoint(37.0, 38.0, 39.0),
                                     DataPoint(40.0, 41.0, 42.0)};
    auto const float_arr_df = array{DataPoint(1.0f, 2.0f, 3.0f),//TODO: Doesn't work with floats
                                    DataPoint(4.0f, 5.0f, 6.0f),
                                    DataPoint(7.0f, 8.0f, 9.0f),
                                    DataPoint(10.0f, 11.0f, 12.0f),
                                    DataPoint(13.0f, 14.0f, 15.0f),
                                    DataPoint(16.0f, 17.0f, 18.0f),
                                    DataPoint(19.0f, 20.0f, 21.0f),
                                    DataPoint(22.0f, 23.0f, 24.0f),
                                    DataPoint(25.0f, 26.0f, 27.0f),
                                    DataPoint(28.0f, 29.0f, 30.0f),
                                    DataPoint(31.0f, 32.0f, 33.0f),
                                    DataPoint(34.0f, 35.0f, 36.0f),
                                    DataPoint(37.0f, 38.0f, 39.0f),
                                    DataPoint(40.0f, 41.0f, 42.0f)};
    auto const int_vec_df = vector{DataPoint(1, 2, 3),
                                   DataPoint(4, 5, 6),
                                   DataPoint(7, 8, 9),
                                   DataPoint(10, 11, 12),
                                   DataPoint(13, 14, 15),
                                   DataPoint(16, 17, 18),
                                   DataPoint(19, 20, 21),
                                   DataPoint(22, 23, 24),
                                   DataPoint(25, 26, 27),
                                   DataPoint(28, 29, 30),
                                   DataPoint(31, 32, 33),
                                   DataPoint(34, 35, 36),
                                   DataPoint(37, 38, 39),
                                   DataPoint(40, 41, 42)};                                    
    print("OUTPUT clusters:\n\n");
    print_clusters(k_means<4>(double_arr_df, 10));

    return 0;
}
