#include <array>
#include <concepts>
#include <algorithm>

namespace kmn {

namespace stdr = std::ranges;

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

  // clang-format off
  constexpr DataPoint(auto... coords) noexcept
    requires(sizeof...(coords) == D)
  : std::array<T, D>{ coords... }
  { }

  [[nodiscard]] friend constexpr
  auto operator<=>(DataPoint const&,
                   DataPoint const&) -> bool = default;

  // operator+ is a hidden friend because:
  // f(a, b) considers implicit conversion for a and b
  // whereas a.f(b) only considers it for b
  // meaning "a + b" and "b + a" might have different behavior
  // Hidden -only found through ADL- friends are preferred for symmetric operations
  [[nodiscard]] friend constexpr
  auto operator+(DataPoint const& lhs,
                 DataPoint const& rhs) -> DataPoint
  {
    DataPoint res;
    stdr::transform(lhs, rhs, res.begin(), std::plus{});
    return res;
  }

  // operator/ overload for floating point value type;
  // result's value type matches it
  [[nodiscard]] constexpr
  auto operator/(arithmetic auto n) const
  -> DataPoint<value_type, D>
    requires(std::floating_point<value_type>)
  {
    DataPoint<value_type, D> res;
    stdr::transform(*this, res.begin(),
                    [n](value_type e)
                    { return e / static_cast<value_type>(n); });
    return res;
  }

  // operator/ overload for integer T => result's value type = double
  [[nodiscard]] constexpr
  auto operator/(arithmetic auto n) const
  -> DataPoint<double, D>
  {
    DataPoint<double, D> res;
    stdr::transform(*this, res.begin(),
                    [n](value_type e)
                    { return e / static_cast<double>(n); });
    return res;
  }

  // clang-format on
  template<std::floating_point U>
  [[nodiscard]] constexpr explicit //
  operator DataPoint<U, D>() const
  {
    DataPoint<double, D> res;
    stdr::transform(*this, res.begin(),
                    [](value_type num) //
                    { return static_cast<U>(num); });
    return res;
  }
};
// DataPoint deduction guide
template<arithmetic T, typename... Us>
DataPoint(T, Us...) -> DataPoint<T, sizeof...(Us) + 1>;

} // namespace kmn
