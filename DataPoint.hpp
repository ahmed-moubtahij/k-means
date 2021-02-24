#include <array>
#include <algorithm>
#include <concepts>

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

    using std::ranges::transform;

    constexpr DataPoint() noexcept = default;
    constexpr DataPoint(DataPoint const&) noexcept = default;
    constexpr DataPoint& operator=(DataPoint const&) noexcept = default;
    constexpr DataPoint(std::convertible_to<value_type> auto... coords) noexcept
        requires (sizeof...(coords) == D)
        : std::array<T, D>{coords...} {}

    friend constexpr bool operator<=>(DataPoint const&, DataPoint const&) = default;

    //operator+ is a hidden friend because:
    //f(a, b) considers implicit conversion for a and b
    //whereas a.f(b) only considers it for b
    //meaning "a + b" and "b + a" might have different behavior
    //Hidden -i.e. only found through ADL- friends are preferred for symmetric operations
    friend constexpr auto
    operator+(DataPoint const& lhs, DataPoint const& rhs)
    -> DataPoint
    {
        DataPoint res;
        transform(lhs, rhs, res.begin(), std::plus{});
        return res;
    }

    //operator/ overload for floating point value type; result's value type matches it
    constexpr auto
    operator/(arithmetic auto n) const
    -> DataPoint<value_type, D>
    requires (std::floating_point<value_type>)
    {
        DataPoint<value_type, D> res;
        transform(*this, res.begin(), [n](value_type e)
            { return e / static_cast<value_type>(n); });
        return res;
    }

    //operator/ overload for integer T => result's value type = double
    constexpr auto
    operator/(arithmetic auto n) const
    -> DataPoint<double, D>
    {
        DataPoint<double, D> res;
        transform(*this, res.begin(), [n](value_type e)
                  { return e / static_cast<double>(n); });
        return res;
    }

    template<std::floating_point U>
    constexpr explicit
    operator DataPoint<U, D>() const
    {
        DataPoint<double, D> res;
        transform(*this, res.begin(), [](value_type num)
                  { return static_cast<U>(num); });
        return res;
    }
};
//DataPoint deduction guide
template<arithmetic T, typename... Us>
DataPoint(T, Us...)->DataPoint<T, sizeof...(Us) + 1>;
