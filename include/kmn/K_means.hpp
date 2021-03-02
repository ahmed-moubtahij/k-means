#include <fmt/ranges.h>
#include <algorithm>
#include <array>
#include <concepts>
#include <kmn/DataPoint.hpp>
#include <numeric>
#include <optional>
#include <range/v3/range/conversion.hpp> //std::ranges doesn't have to(_container)
#include <range/v3/view/filter.hpp> //Used when pipe chain contains a range-v3 adaptor
#include <range/v3/view/map.hpp> //Used when pipe chain contains a range-v3 adaptor
#include <range/v3/view/sample.hpp> //Used over std::ranges::sample for pipability & lazy semantics
#include <range/v3/view/transform.hpp> //Used when pipe chain contains a range-v3 adaptor
#include <range/v3/view/zip.hpp> //std::ranges doesn't have zip
#include <ranges>
#include <vector>

#define FWD(x) static_cast<decltype(x)&&>(x)

namespace kmn {

using size_type = std::size_t;

namespace stdr = std::ranges;
namespace stdv = std::views;
namespace r3 = ranges; // range-v3
namespace r3v = r3::views;

using stdr::range_value_t;
using stdv::filter;

using r3v::zip;

using std::declval;
using std::vector;

namespace hlpr {
  /************************ select_centroid ******************************/
  // clang-format off
  
  // if T is integral, picks a floating value type for the centroid
  template<typename T, size_type D> //
  struct select_centroid
  { using type = DataPoint<double, D>; };
  // if T is a floating point type, the centroid's value type matches it
  template<std::floating_point T, size_type D> //
  struct select_centroid<T, D>
  { using type = DataPoint<T, D>; };
  // T is pre-constrained to being an arithmetic type in DataPoint
  template<typename T, size_type D>
  using select_centroid_t = typename select_centroid<T, D>::type;
  
  /************************* is_data_point *******************************/
  
  template<typename T> struct is_data_point: std::false_type { };

  template<typename T, size_type D>
  struct is_data_point<DataPoint<T, D>>: std::true_type { };
  
  template<typename T>
  inline constexpr bool is_data_point_v = is_data_point<T>::value;
  /************************ data_point_size ******************************/
  template<typename T> struct data_point_size;
  
  template<typename T, size_type D>
  struct data_point_size<DataPoint<T, D>>:
  std::integral_constant<size_type, D> { };

  // clang-format on
  template<typename T>
  inline constexpr size_type data_point_size_v = data_point_size<T>::value;

  /************************** data_point_t *******************************/
  template<typename R> using data_point_t = range_value_t<R>;
  /************************** point_value_t ******************************/
  template<typename R>
  using point_value_t = typename data_point_t<R>::value_type;
  /***************************** Concepts ********************************/
  template<typename R>
  concept data_points_range = is_data_point_v<range_value_t<R>>;
  template<typename R>
  concept unsigned_range = std::unsigned_integral<range_value_t<R>>;
} // namespace hlpr

using namespace hlpr;

// sqr_dist: computes euclidean square distance between two data points
template<typename T1, typename T2, size_type D>
constexpr auto sqr_distance(DataPoint<T1, D> const& dp1,
                            DataPoint<T2, D> const& dp2)
{
  return std::transform_reduce(
  dp1.cbegin(), dp1.cend(), dp2.cbegin(), 0,
  [](T1 a, T2 b) { return a + b; },
  [](T1 a, T2 b) { return (a - b) * (a - b); });
}

// distance_from: Function Object Comparator
//               of distances from two points to a reference point
template<typename T, size_type D> //
struct distance_from
{
  using target_point_t = DataPoint<T, D>;
  target_point_t m_pt;

  distance_from() = delete;
  distance_from& operator=(distance_from const&) = delete;

  constexpr distance_from(target_point_t const& pt) //
  : m_pt{ pt }
  { }

  // clang-format off
  template<typename U>
  constexpr bool operator()(DataPoint<U, D> const& c1,
                            DataPoint<U, D> const& c2) const
  { return sqr_distance(c1, m_pt) < sqr_distance(c2, m_pt); }
}; // struct distance_from

// clang-format on
template<typename PTS_R>
using centroid_t =
select_centroid_t<point_value_t<PTS_R>,
                  data_point_size_v<data_point_t<PTS_R>>>;

template<typename PTS_R>
using indexed_centroids_t =
vector<std::pair<size_type, centroid_t<PTS_R>>>;

template<typename PTS_R>
auto init_centroids(PTS_R&& data_points, size_type k)
-> indexed_centroids_t<PTS_R>
{
  using pt_value_t = point_value_t<PTS_R>;
  using r3v::sample, r3::to;
  using id_centroids_t = indexed_centroids_t<PTS_R>;
  using centroids_t =
  vector<typename id_centroids_t::value_type::second_type>;

  // Initialize centroid ids
  auto const ids = stdv::iota(size_type{ 1 }, k + 1);

  // Initialize centroids with a sample of K points from data_points
  if constexpr(std::floating_point<pt_value_t>) //
  {
    auto const centroids =
    FWD(data_points) | sample(k) | to<centroids_t>();

    return zip(ids, centroids) | to<id_centroids_t>();

  } else {
    // data_points here has integral value types 'T'
    // centroids (which get updated with means) have floating point value
    // types So the sampled points' value types need to match
    using data_pt_t =
    DataPoint<pt_value_t, data_point_size_v<data_point_t<PTS_R>>>;

    auto constexpr cast_to_cendroid_t = [](data_pt_t const& pt)
    { return static_cast<centroid_t<PTS_R>>(pt); };

    auto const centroids = FWD(data_points)
                         | r3v::transform(cast_to_cendroid_t) | sample(k)
                         | to<centroids_t>();

    return zip(ids, centroids) | to<id_centroids_t>();
  }
}

// match_id: Used in update_centroids and in k_means_result
struct match_id
{
  size_type cent_id;
  // clang-format off
  constexpr bool operator()(auto const& indexed_point) const
  { return cent_id == std::get<0>(indexed_point); }
  
}; //struct match_id

// clang-format on
void update_centroids(auto&& data_points,
                      auto&& out_indices,
                      auto&& indexed_centroids)
{
  auto constexpr mean_matching_points = [](auto&& points)
  {
    using data_point_t = range_value_t<decltype(points)>;
    size_type count{};
    // an elements' count is used instead of size()
    // because the input range is a filter_view which is not a sized_range
    auto const counted_sum =
    [&count](data_point_t const& pt1, data_point_t const& pt2)
    { return (void(++count), pt1 + pt2); };
    // cast to void protects against "operator," overloads

    // sum must be calculated separately
    // because count is a side effect of counted_sum
    auto const sum = //
    std::reduce(stdr::begin(points), stdr::end(points), //
                data_point_t(), counted_sum);

    return sum / count;
  };

  using stdr::transform, r3v::keys, r3v::values, r3::begin, r3v::filter;

  transform(keys(indexed_centroids),
            begin(values(indexed_centroids)),
            [&](auto const& cent_id) { //
              return mean_matching_points(
              zip(FWD(out_indices), FWD(data_points))
              | filter(match_id{ cent_id }) //
              | values);
            });
}

void index_points_by_centroids(auto&& out_indices,
                               auto&& data_points,
                               auto const& indexed_centroids)
{
  auto constexpr dist_from_centroid = //
  [](auto const& pt) { return distance_from{ pt }; };

  auto constexpr proj_centroid = //
  [](auto const& id_pt) { return id_pt.second; };

  // clang-format off
  auto const find_id_nearest_centroid =
  [&](auto const& pt) // stdr::borrowed_iterator has no operator->
  { 
    return (*stdr::min_element(indexed_centroids,
                               dist_from_centroid(pt),
                               proj_centroid)).first;
  };
  
  // Find the ID of the nearest centroid to each data point,
  // and write it to the output range
  using stdr::transform, std::begin;

  transform(FWD(data_points),
            begin(FWD(out_indices)),
            find_id_nearest_centroid);
}
// clang-format on

auto clusters_histogram(auto const& indices, size_type k)
{
  std::vector<size_type> cluster_sizes(k);
  for(auto i: indices) ++cluster_sizes[i - 1];
  return cluster_sizes;
}

template<typename CENTROIDS_R,
         typename SIZES_R, //
         typename INPUT_R, //
         typename OUTPUT_R>
class k_means_result {
  CENTROIDS_R m_centroids;
  SIZES_R m_cluster_sizes;
  INPUT_R m_points;
  OUTPUT_R m_out_indices;

  static constexpr auto filter = r3v::filter;
  static constexpr auto values = r3v::values;

  using indexed_range = decltype(zip(FWD(m_out_indices), FWD(m_points)));

  using filtered_indexed_range =
  decltype(declval<indexed_range>() | filter(match_id{}));

  using filtered_range =
  decltype(declval<filtered_indexed_range>() | values);

  struct iterator
  {
    k_means_result& parent;
    size_type cluster_idx;

    // clang-format off
    struct cluster 
    { range_value_t<CENTROIDS_R>& centroid;
      filtered_range satellites;
    }; // struct k_means_result::iterator::cluster
    
    auto operator*() const -> cluster
    {
      return { parent.m_centroids[cluster_idx],
               zip(FWD(parent.m_out_indices), FWD(parent.m_points))
               | filter(match_id{ cluster_idx + 1 })
               | values };
    }

    void operator++(){ ++cluster_idx; }

    friend bool operator==(iterator lhs, iterator rhs)
    { return lhs.cluster_idx == rhs.cluster_idx; }
    
  }; // struct k_means_result::iterator

  // clang-format on
public:
  k_means_result(CENTROIDS_R centroids,
                 SIZES_R cluster_sizes,
                 INPUT_R points,
                 OUTPUT_R out_indices)
  : m_centroids{ centroids }, //
    m_cluster_sizes{ cluster_sizes }, //
    m_points{ points }, //
    m_out_indices{ out_indices }
  { }

  // clang-format off
  auto centroids() const -> CENTROIDS_R
  { return m_centroids; }

  auto cluster_sizes() const -> SIZES_R
  { return m_cluster_sizes; }

  auto points() const -> INPUT_R
  { return m_points; }

  auto out_indices() const -> OUTPUT_R
  { return m_out_indices; }

  auto begin() -> iterator
  { return { *this, size_type{ 0 } }; }
  
  auto end() -> iterator
  { return { *this, m_cluster_sizes.size() }; }
  
}; // struct k_means_result

// clang-format on
void print_kmn_result(auto&& optional_kmn_result)
{
  using fmt::print;

  auto const decorator_width = 77;

  auto constexpr print_block =
  [decorator_width](std::string_view title, auto&& printable)
  {
    print("{:-^{}}\n", title, decorator_width);
    print("\n{}\n\n", printable);
  };

  auto&& kmn_result = *FWD(optional_kmn_result);

  print_block(" Input data points ", //
              kmn_result.points());
  print_block(" Cluster indices for each point ",
              kmn_result.out_indices());
  print_block(" Centroids ", //
              kmn_result.centroids());
  print_block(" Cluster Sizes ", //
              kmn_result.cluster_sizes());

  print("{:*^{}}\n\n", " CLUSTERS ", decorator_width);

  for(std::size_t i{ 1 }; //
      auto&& [centroid, satellites]: kmn_result)
  {
    print("{:-^{}}\n", //
          fmt::format(" Centroid {}: {} ", i++, centroid),
          decorator_width);

    using satellite_t = range_value_t<decltype(satellites)>;
    using r3::to;
    print("\n{}\n\n", FWD(satellites) | to<vector<satellite_t>>);
    // ranges::to<container> conversion is needed for fmt 7.0.0, it won't be after
  }
}

template<typename IDX_R>
using cluster_sizes_t =
decltype(clusters_histogram(declval<IDX_R>(), declval<size_type>()));

template<typename PTS_R, typename IDX_R>
using k_means_impl_t = k_means_result<vector<centroid_t<PTS_R>>, //
                                      cluster_sizes_t<IDX_R>, //
                                      PTS_R, IDX_R>;

template<typename PTS_R, typename IDX_R>
constexpr auto k_means_impl(PTS_R&& data_points, //
                            IDX_R&& out_indices, //
                            size_type k, size_type n)
-> k_means_impl_t<PTS_R, IDX_R>
{ // Initialize centroids and their ids
  auto&& indexed_centroids = init_centroids(FWD(data_points), k);

  index_points_by_centroids(FWD(out_indices), FWD(data_points),
                            FWD(indexed_centroids));

  // Update the centroids with means, repeat n times
  while(n--) {
    update_centroids(FWD(data_points), FWD(out_indices),
                     FWD(indexed_centroids));
  }
  return { (r3v::values(indexed_centroids)
            | r3::to<vector<centroid_t<PTS_R>>>()),
           clusters_histogram(FWD(out_indices), k), //
           FWD(data_points), //
           FWD(out_indices) };
}

struct k_means_fn
{
  template<data_points_range PTS_R, unsigned_range IDX_R>
  constexpr auto operator()(
  PTS_R&& data_points, // Not mutated but a reference to it is
  // returned; Will be moved if it's an rvalue
  // so as to not return a dangling reference
  IDX_R&& out_indices, // Is an rvalue reference instead of lvalue
  // reference to handle rvalue arguments such
  // as views-like objects
  size_type k, size_type n) const noexcept
  -> std::optional<k_means_impl_t<PTS_R, IDX_R>>
  {
    if(k < 2) return std::nullopt;

    // distance() is used instead of size() to support non-sized range
    // arguments such as "data_points_arg | filter(...)"
    if(auto const pts_dist = stdr::distance(data_points);
       not std::in_range<size_type>(pts_dist))
    {
      return std::nullopt; // Fall if distance(data_points) is signed
    } //
    else if(auto const pts_size = static_cast<size_type>(pts_dist);
            pts_size < k or pts_size != stdr::size(out_indices))
    {
      return std::nullopt;
    }

    return { k_means_impl<PTS_R, IDX_R>(FWD(data_points), //
                                        FWD(out_indices), //
                                        k, n) };
  }
};

// k_means: Callable object that the user can call or pass around
constexpr inline k_means_fn k_means{};

} // namespace kmn
