#include <concepts>
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <ranges>
#include <span>
#include <fmt/ranges.h>

#define FWD(x) static_cast<decltype(x)&&>(x)

using fmt::print;

//TODO: Find a minimized use case for dispatch to present it for discussion
//      Dispatch generalization e.g. push_back restricts to vectors

template<typename ...Ts>
[[deprecated]] constexpr bool print_type = true;

//dispatch: Akin to a partition_copy with output to multiple ranges
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

    constexpr DataPoint() noexcept = default;
    constexpr DataPoint& operator=(DataPoint const&) noexcept = default;
    constexpr DataPoint(std::convertible_to<value_type> auto... coords) noexcept
        requires (sizeof...(coords) == D)
        : std::array<T, D>{coords...} {}
    
    friend constexpr bool operator<=>(DataPoint const&, DataPoint const&) = default;
    
    constexpr auto operator+(DataPoint const& rhs) const
    {
        DataPoint res;
        for(std::size_t i{}; i < D; ++i)
            res[i] = rhs[i] + (*this)[i];
        return res;
    }

    constexpr auto operator/(arithmetic auto n)
    {
        DataPoint<double, D> res;
        for(std::size_t i{}; i < D; ++i)
            res[i] = (*this)[i] / static_cast<double>(n);
        return res;
    }
};
//DataPoint deduction guide
template<arithmetic T, typename... Us>
DataPoint(T, Us...) -> DataPoint<T, sizeof...(Us) + 1>;

//Aggregate template Cluster
template<typename T, std::size_t D>
struct Cluster{
    using centroid_t = DataPoint<T, D>;
    using satellites_t = std::vector<DataPoint<T, D>>; 
    
    centroid_t centroid;    
    satellites_t satellites;

    friend bool operator<=>(Cluster const&, Cluster const&) = default;
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
auto init_centroids(auto&& clusters, auto const& data)
{
    using cluster_t = Cluster<T, D>;    
    auto centroids = FWD(clusters) | rnv::transform(&cluster_t::centroid);
                    
    //Initialize centroids with a sample of K points from data
    rn::sample(data, centroids.begin(),
               K, std::mt19937{std::random_device{}()});
    return centroids;
}

template<typename T, std::size_t D>
void update_satellites(auto&& non_centroid_data,
                       auto&& clusters)
{
    using cluster_t = Cluster<T, D>;
    
    rn::for_each(clusters,
                 [](auto&& satellites){ FWD(satellites).clear();},
                 &cluster_t::satellites);
    
    auto constexpr find_closest_centroid =
    [](auto&& clusters, auto comp, auto proj = std::identity{})
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
void update_centroids(auto const& clusters, auto&& centroids)
{
    using cluster_t = Cluster<T, D>;
    
    auto constexpr mean = [](auto const& r)
    {   auto constexpr identity_element =
        typename std::remove_cvref_t<decltype(r)>::value_type();
        
        return std::reduce(r.cbegin(), r.cend(), identity_element,
                           std::plus{}) / r.size();
    };        
    auto const closest_to_mean = [&mean](auto const& sats_range)
    { return *rn::min_element(sats_range, distance_from{ mean(sats_range) }); };

    auto constexpr has_satellites = [](auto const& cluster)
    { return not cluster.satellites.empty(); };
    //Update every centroid with its satellites mean
    rn::transform(clusters | rnv::filter(has_satellites),
                  FWD(centroids).begin(),
                  closest_to_mean, &cluster_t::satellites);
}

template<std::size_t K, std::size_t SZ,
         arithmetic T, std::size_t D>
constexpr auto
k_means(std::array<DataPoint<T, D>, SZ> const& data, std::size_t n)
-> std::array<Cluster<T, D>, K>
{
    if (SZ < K or K < 2) return {{}};

    using cluster_t = Cluster<T, D>;
    std::array<cluster_t, K> clusters;
    //A given cluster will have at most SZ satellites
    rn::for_each(clusters,
                 [](auto&& satellites){FWD(satellites).reserve(SZ); },
                 &cluster_t::satellites);

    auto centroids = init_centroids<T, D, K>(clusters, data);
        
    auto const is_not_centroid = [&centroids](auto const& pt)
    { return rn::find(centroids, pt) == centroids.end();};
   
    update_satellites<T, D>(data | rnv::filter(is_not_centroid), clusters);

    while(n--){
        update_centroids<T, D>(clusters, centroids);
        update_satellites<T, D>(data | rnv::filter(is_not_centroid), clusters);
    }
    
    return clusters;
}

} // namespace kmn

int main(){
    using std::array, kmn::DataPoint;
    using kmn::print_clusters, kmn::k_means;

    auto const df = array{DataPoint(1, 2, 3),
                          DataPoint(4, 5, 6),
                          DataPoint(7, 8, 9),
                          DataPoint(10, 11, 12),
                          DataPoint(13, 14, 15),
                          DataPoint(16, 17, 18),
                          DataPoint(19, 20, 21),
                          DataPoint(22, 23, 24)};
    
    print("OUTPUT clusters:\n\n");
    print_clusters(k_means<4>(df, 100));

    return 0;
}
//My thanks to the #includecpp Discord's folk:
//Sarah, Léo, marcorubini, oktal
