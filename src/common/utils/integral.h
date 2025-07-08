#pragma once

#include <type_traits>

namespace uh::cluster {

template <typename T> constexpr T div_ceil(T x, T y) {
    static_assert(std::is_integral<T>::value,
                  "div_ceil only supports integral types");
    return x / y + (x % y != 0);
}

template <typename T> constexpr T align_up(T x, T y) {
    static_assert(std::is_integral<T>::value,
                  "div_ceil only supports integral types");
    return div_ceil(x, y) * y;
}

template <typename T> constexpr T div_floor(T x, T y) {
    static_assert(std::is_integral<T>::value,
                  "div_floor only supports integral types");
    return x / y;
}

template <typename T> constexpr T align_up_next(T x, T y) {
    static_assert(std::is_integral<T>::value,
                  "align_up_next only supports integral types");
    return (div_floor(x, y) + 1) * y;
}

} // namespace uh::cluster
