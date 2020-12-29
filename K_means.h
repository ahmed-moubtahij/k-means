#include <concepts>
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <ranges>
#include <fmt/ranges.h>

#define FWD(x) static_cast<decltype(x)&&>(x)

using fmt::print;

template<typename ...Ts>
[[deprecated]] constexpr bool print_type = true;

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
struct DataPoint : private std::array<T, D> {
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
//DataPoint deduction duide
template<arithmetic T, typename... Us>
DataPoint(T, Us...) -> DataPoint<T, sizeof...(Us) + 1>;

//Aggregate template Cluster
template<typename T, std::size_t D>
struct Cluster{
    DataPoint<T, D> centroid;
    std::vector<DataPoint<T, D>> satellites;

    friend bool operator<=>(Cluster const&, Cluster const&) = default;
};

template<typename T1, typename T2, std::size_t D>
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

//Function Object Comparator distance_from
template<typename REF_PT_T, std::size_t D>
struct distance_from{
    using data_point_t = DataPoint<REF_PT_T, D>;
    data_point_t m_pt;
    
    distance_from() = delete;
    distance_from& operator=(distance_from const&) = delete;
    constexpr distance_from(data_point_t const& pt) : m_pt{pt} {}
    constexpr bool operator()(DataPoint<auto, D> const& c1,
                              DataPoint<auto, D> const& c2) const
    {
        auto const dist_c1_pt = sqr_distance(c1, m_pt);
        auto const dist_c2_pt = sqr_distance(c2, m_pt);
        if (dist_c1_pt == 0) return false; //if m_pt is c1, then c1 < c2 is false
        if (dist_c2_pt == 0) return true;  //if m_pt is c2, then c1 < c2 is true
        return dist_c1_pt < dist_c2_pt;
    }
};

template<std::ranges::input_range R1,
         std::ranges::input_range R2,
         typename F,
         typename Comp = std::ranges::less,
         typename Proj1 = std::identity,
         typename Proj2 = std::identity>
constexpr void dispatch(R1&& r, R2&& buckets,
                        F dispatcher, Comp comp = {},
                        Proj1 proj1 = {}, Proj2 proj2 = {})
{
    for(auto const& e: FWD(r)){
        auto& target_bucket = *dispatcher(FWD(buckets), comp(e), proj1);
        std::invoke(proj2, target_bucket).push_back(e);
    }
}

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
void update_satellites(auto&& clusters,
                       auto const& data,
                       auto const& centroids)
{
    using cluster_t = Cluster<T, D>;
    auto constexpr find_closest_centroid =
    [](auto&& clusters, auto comp, auto proj)
    { return rn::min_element(FWD(clusters), comp, proj); };

    auto const is_not_centroid = [&centroids](auto const& pt)
    { return rn::find(centroids, pt) == centroids.end();};
    
    for(auto&& [_, satellites]: FWD(clusters)) satellites.clear();
    //Dispatch every data point to the cluster whose centroid is closest
    //TODO: Unclear at call site what dispatch is dispatching to
    dispatch(data | rnv::filter(is_not_centroid),
             FWD(clusters), find_closest_centroid,
             [](auto const& pt){ return distance_from{pt}; },
             &cluster_t::centroid, &cluster_t::satellites);
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
    auto const closest_to_mean = [&mean](auto const& r)
    { return *rn::min_element(r, distance_from{ mean(r) }); };

    auto constexpr has_satellites = [](auto const& cluster)
    { return not cluster.satellites.empty(); };
    //Update every centroid with its satellites mean
    rn::transform(clusters | rnv::filter(has_satellites),
                  FWD(centroids).begin(),
                  closest_to_mean, &cluster_t::satellites);
}

template<std::size_t K, arithmetic T,
        std::size_t SZ, std::size_t D>
constexpr auto
k_means(std::array<DataPoint<T, D>, SZ> const& data, std::size_t n)
-> std::array<Cluster<T, D>, K>
{
    if (SZ < K) return {{}};

    using cluster_t = Cluster<T, D>;
    std::array<cluster_t, K> clusters;
    //A given cluster will have at most SZ satellites
    for(auto&& [_, sats] : clusters) sats.reserve(SZ);
    
    auto centroids = init_centroids<T, D, K>(clusters, data);
    //TODO: Unclear at call site which arg update_satellites updates
    update_satellites<T, D>(clusters, data, centroids);
    
    while(n--)
    {
        update_centroids<T, D>(clusters, centroids);
        update_satellites<T, D>(clusters, data, centroids);
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
    
    print("INPUT data points:\n\n{}\n\n", df);
    
    print("OUTPUT clusters:\n\n");
    print_clusters(k_means<3>(df, 50));
    // k_means<3>(df, 10);

    return 0;
}
//My thanks to the #includecpp Discord's folk:
//Sarah, Léo, marcorubini, oktal
