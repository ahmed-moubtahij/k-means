#include <concepts>
#include <algorithm>
#include <array>
#include <cassert>
#include <range/v3/view/transform.hpp>
#include <type_traits>

namespace kmn {

namespace stdr = std::ranges;
namespace rng = ranges;
namespace rnv = rng::views;

template<typename T>
concept arithmetic = std::integral<T> or std::floating_point<T>;

// Struct template DataPoint
template<arithmetic T, std::size_t D>
struct DataPoint final: private std::array<T, D>
{
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
  constexpr DataPoint(DataPoint&&) noexcept = default;
  constexpr DataPoint& operator=(DataPoint&&) noexcept = default;

  // clang-format off
  template<std::convertible_to<value_type> ...Ts>
  constexpr explicit(sizeof...(Ts) == 1)
  DataPoint(Ts... coords) noexcept
    requires(sizeof...(coords) == D)
  : std::array<T, D>{ coords... }
  { }

  template<stdr::sized_range R>
  constexpr explicit
  DataPoint(R r) noexcept
    requires(std::convertible_to<stdr::range_value_t<R>, value_type>)
  {
    assert(stdr::size(r) == D);
    stdr::copy(std::move(r), begin());
  }

  [[nodiscard]] friend constexpr
  auto operator+(DataPoint const& lhs,
                 DataPoint const& rhs)
  {
    return DataPoint(rng::transform2_view(lhs, rhs, std::plus{}));
  }

  [[nodiscard]] constexpr
  auto operator/(arithmetic auto n) const
  {
    return DataPoint<double, D>
           (rnv::transform(*this, 
            [n](T e){ return e / static_cast<double>(n); }));
  }

};
// DataPoint deduction guide
template<arithmetic T, typename... Us>
DataPoint(T, Us...) -> DataPoint<T, sizeof...(Us) + 1>;

} // namespace kmn

// Customization point for specifying the cardinality of ranges
// range-v3/include/range/v3/range/traits.hpp#L121
// This specialization is needed by views::transform and ranges::transform2_view
// for DataPoint to retain *private* inheritance; ranges::test_cardinality tries to access it
// Filed bug under https://github.com/ericniebler/range-v3/issues/1614
template<typename T, std::size_t D>
struct ranges::range_cardinality<kmn::DataPoint<T, D>>
    : std::integral_constant<ranges::cardinality,
                             static_cast<ranges::cardinality>(D)>
{ };

template<typename T, std::size_t D>
struct ranges::range_cardinality<kmn::DataPoint<T, D> const>
    : ranges::range_cardinality<kmn::DataPoint<T, D>>
{ };
