#include <concepts>
#include <array>
#include <vector>
#include <algorithm>
#include <random>
#include <numeric>
#include <memory>
#include <ranges>
#include <range/v3/view/zip.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/map.hpp> //ranges::views::values
#include <fmt/ranges.h>

#define FWD(x) static_cast<decltype(x)&&>(x)

using fmt::print;

namespace kmn{

namespace rn = std::ranges;
namespace rnv = rn::views;
namespace rv3 = ranges::views;

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
    { DataPoint res;
      rn::transform(rhs, *this, res.begin(), std::plus{});
      return res;
    }

    //operator/ overload for floating point value type; result's value type matches it
    constexpr auto operator/(arithmetic auto n) const
        requires (std::floating_point<value_type>)
    { DataPoint<value_type, D> res;
      rn::transform(*this, res.begin(),
      [&n](value_type e){ return e / static_cast<value_type>(n); });
      return res;
    }

    //operator/ overload for integer T => result's value type = double
    constexpr auto operator/(arithmetic auto n) const
    { DataPoint<double, D> res;
      rn::transform(*this, res.begin(),
                    [&n](value_type e)
                    { return e / static_cast<double>(n); });
      return res;
    }

    template<std::floating_point U>
    constexpr explicit operator DataPoint<U, D>() const
    { DataPoint<double, D> res;
      rn::transform(*this, res.begin(),
                    [](value_type num)
                    { return static_cast<U>(num); });
      return res;
    }
};
//DataPoint deduction guide
template<arithmetic T, typename... Us>
DataPoint(T, Us...) -> DataPoint<T, sizeof...(Us) + 1>;

void make_clusters(auto const& kmn_result)
{ auto&& [centroids, cluster_sizes] = kmn_result;
  using cluster_t = DataPoint<int, 3>; //MOCK
  std::vector<cluster_t> clusters(cluster_sizes.size());
  //TODO: reserve cluster_size for each cluster
}

void print_kmn_result(auto const& kmn_result)
{ 
  //TODO: Print a text of Centroids | Cluster sizes | Satellites
  for(auto const& [centroids, cluster_sizes, discard_1, discard_2] = kmn_result;
      auto const& [centroid, cluster_size]: rv3::zip(centroids, cluster_sizes))
  { print("Centroid: {}\nCluster size: {}\n\n", centroid, cluster_size); }
    //TODO: kmn_result destructuring problem
    print("Calling print_kmn_result()\n");
}

namespace hlpr{
    template<typename T, std::size_t D>
    struct select_centroid
    //A DataPoint's value type T is constrained to be arithmetic
    //So T here is an integral if the floating_point overload is discarded
    { using type = DataPoint<double, D>; };

    template<std::floating_point T, std::size_t D>
    struct select_centroid<T, D>
    { using type = DataPoint<T, D>; };

    template<typename T, std::size_t D>
    using select_centroid_t = typename select_centroid<T, D>::type;
}

//sqr_dist: computes euclidean square distance between two data points
template<typename T1, typename T2, std::size_t D>
constexpr auto
sqr_distance(DataPoint<T1,D> const& dp1,
             DataPoint<T2,D> const& dp2)
{    return std::transform_reduce(
            dp1.cbegin(), dp1.cend(), dp2.cbegin(), 0,
            [](T1 a, T2 b){ return a + b; },
            [](T1 a, T2 b){ return (a - b)*(a - b); });
};
//distance_from: Function Object Comparator
//               of distances from two points to a reference point
template<typename T, std::size_t D>
struct distance_from
{   using ref_point_t = DataPoint<T, D>;
    ref_point_t m_pt;
    
    distance_from() = delete;
    distance_from& operator=(distance_from const&) = delete;
    constexpr distance_from(ref_point_t const& pt) : m_pt{pt} {}
    
    template<typename U>
    constexpr bool operator()(DataPoint<U, D> const& c1,
                              DataPoint<U, D> const& c2) const
    { return sqr_distance(c1, m_pt) < sqr_distance(c2, m_pt); }
};

template<typename T, std::size_t D>
void init_centroids(auto&& centroids,
                    auto const& data_points,
                    std::size_t k)
{   //Initialize centroids with a sample of K points from data_points
    if constexpr(std::floating_point<T>){
        rn::sample(data_points,
                   rn::begin(centroids), k,
                   std::mt19937{std::random_device{}()});
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

namespace hlpr
{   template<typename T>
    struct is_data_point : std::false_type {};
    template<typename T, std::size_t D>
    struct is_data_point<DataPoint<T, D>> : std::true_type {};
    
    template<typename T>
    struct data_point_size;
    template<typename T, std::size_t D>
    struct data_point_size<DataPoint<T, D>> : std::integral_constant<std::size_t, D> {};
}

template<typename R>
concept data_points_range = hlpr::is_data_point<rn::range_value_t<R>>::value;

//This is used in update_centroids and in k_means_result
struct match_id
{ std::size_t cent_id;
  constexpr auto
  operator()(auto const& indexed_point) const -> bool
  { return cent_id == get<0>(indexed_point); }
};

void update_centroids(auto const& data_points,
                      auto&& out_indices,
                      auto&& indexed_centroids)
{   auto constexpr mean_matching_points =
    [](data_points_range auto&& data_points)
    {   using data_point_t = rn::range_value_t<decltype(data_points)>;
        std::size_t count{};
        //input range is a filter_view which is not a sized range
        //so an elements' count is used instead of size()
        auto const counted_sum = 
        [&count](data_point_t const& pt1, data_point_t const& pt2)
        { ++count; return pt1 + pt2; };
        //count is a side effect of counted_sum,
        //so sum must be calculated separately
        auto const sum = std::reduce(rn::begin(data_points), rn::end(data_points),
                                     data_point_t(), counted_sum);
        return sum / count;
    };

    auto const indexed_points = rv3::zip(FWD(out_indices), data_points);

    rn::transform(FWD(indexed_centroids) | rnv::keys,
                  rn::begin(FWD(indexed_centroids) | rnv::values),
                  [&](auto const& cent_id)
                  { return mean_matching_points(indexed_points
                                                | rnv::filter(match_id{cent_id})
                                                | rnv::values);
                  });
}

void index_points_by_centroids(auto&& out_indices,
                               auto const& data_points,
                               auto const& indexed_centroids)
{   auto constexpr dist_from_centroid =
    [](auto const& pt){ return distance_from{ pt }; };

    auto constexpr proj_centroid =
    [](auto const& id_pt){ return id_pt.second; };

    auto const find_id_nearest_centroid =
    [&](auto const& pt) //rn::borrowed_iterator has no operator->
    {   return (*rn::min_element(indexed_centroids,
                                 dist_from_centroid(pt),
                                 proj_centroid)).first;
    };
    //Find the ID of the nearest centroid to each data point,
    //and write it to the output range
    rn::transform(data_points,
                  std::begin(FWD(out_indices)),
                  find_id_nearest_centroid);
}

void set_cluster_sizes(auto&& cluster_sizes,
                            auto const& centroid_ids,
                            auto const& out_indices)
{   auto const count_indices =
    [&out_indices](std::size_t index)
    { return rn::count(out_indices, index); };

    rn::transform(centroid_ids,
                  FWD(cluster_sizes).begin(),
                  count_indices);
}

template <typename CENTROIDS_R, typename SIZES_R,
          typename Input_Range, typename Output_Range>
struct k_means_result {
  //TODO: Have generic types for these members
  //      so as to not risk type mismatch with call site
  CENTROIDS_R centroids;
  SIZES_R sizes;
  Input_Range points;
  Output_Range indices;

    using indexed_range = decltype(rv3::zip(*indices, *points));
    using filtered_indexed_range =
    decltype(rv3::filter(std::declval<indexed_range>(),
                         match_id{0}));
    using filtered_range =
    decltype(rv3::values(std::declval<filtered_indexed_range>()));

    struct proxy
    { typename rn::range_value_t<CENTROIDS_R> const& centroid;
      filtered_range cluster;
    };

    struct iterator
    { k_means_result& parent;
      size_t cluster_idx;
  
      auto operator*() const -> proxy
      { return {parent.centroids[cluster_idx],
                  (rv3::zip(parent.indices, parent.points)
                  | rnv::filter(match_id{cluster_idx})
                  | rnv::values)};
    }

public:
    auto operator++() -> iterator& { ++cluster_idx; }
    friend auto operator==(iterator lhs, iterator rhs) -> bool = default;
    // etc
    };


    auto begin() const -> iterator { return {*this, 0}; }
    auto end() const -> iterator { return {*this, sizes.size()}; }
};

template<typename PT_VALUE_T, std::size_t D>
using centroid_t = hlpr::select_centroid_t<PT_VALUE_T, D>;

template<typename PT_VALUE_T, std::size_t D>
constexpr auto
k_means_impl(auto const& data_points,
             auto&& out_indices,
             std::size_t k, std::size_t n)
-> k_means_result<std::vector<centroid_t<PT_VALUE_T, D>>,
                  std::vector<std::size_t>,
                  std::remove_cvref_t<decltype(data_points)> const*,
                  std::remove_cvref_t<decltype(out_indices)>*>
{   if(k < 2) return {};
    
    if(auto const pts_size = rn::distance(data_points);
       pts_size < k or pts_size != rn::size(out_indices))
        return {};

    std::vector<std::size_t> cluster_sizes(k);
    
    std::vector<centroid_t<PT_VALUE_T, D>> centroids(k);
    std::vector<std::size_t> ids(k);
    auto indexed_centroids = rv3::zip(ids, centroids);
    
    //Initialize centroid ids
    auto centroid_ids = rnv::keys(indexed_centroids);
    std::iota(std::begin(centroid_ids), std::end(centroid_ids), 1);

    init_centroids<PT_VALUE_T, D>(rnv::values(indexed_centroids), data_points, k);

    index_points_by_centroids(out_indices, data_points, indexed_centroids);

    //Update the centroids with means, repeat n times
    while(n--)
    { update_centroids(data_points,
                       out_indices,
                       indexed_centroids);
    }

    set_cluster_sizes(cluster_sizes, centroid_ids, out_indices);

    return k_means_result{centroids, cluster_sizes, &data_points, &out_indices};
}

struct k_means_fn
{ constexpr auto operator()
  (data_points_range auto const& data_points,
   std::vector<size_t>& out_indices, 
   std::size_t k, std::size_t n) const
  { using data_point_t = rn::range_value_t<decltype(data_points)>;
    using point_value_t = data_point_t::value_type;
    
    auto constexpr dimension = hlpr::data_point_size<data_point_t>::value;   
    
    return k_means_impl<point_value_t, dimension>(FWD(data_points),
                                                   out_indices, k, n);
  }
};

//k_means: Callable object that the user can call or pass around
constexpr inline kmn::k_means_fn k_means{};

} // namespace kmn

int main(){
    using std::array, std::vector, kmn::DataPoint;
    using kmn::print_kmn_result, kmn::k_means;

    //INPUT ranges
    auto const int_arr_df =
    array{DataPoint(1, 2, 3),    DataPoint(4, 5, 6),    DataPoint(7, 8, 9),
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
    vector{DataPoint(1, 2, 3),    DataPoint(4, 5, 6),    DataPoint(7, 8, 9),
           DataPoint(10, 11, 12), DataPoint(13, 14, 15), DataPoint(16, 17, 18),
           DataPoint(19, 20, 21), DataPoint(22, 23, 24), DataPoint(25, 26, 27),
           DataPoint(28, 29, 30), DataPoint(31, 32, 33), DataPoint(34, 35, 36),
           DataPoint(37, 38, 39), DataPoint(40, 41, 42), DataPoint(43, 44, 45)};

    //OUTPUT range
    std::vector<std::size_t> out_indices(int_arr_df.size());
    
    //CALL & DISPLAY RESULT
    std::size_t const k{ 4 }, n{ 10 };
    print_kmn_result(k_means(int_arr_df, out_indices, k, n));
    // k_means(int_arr_df, out_indices, k, n)

    return 0;
}
