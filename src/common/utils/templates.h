#ifndef CORE_COMMON_TEMPLATES_H
#define CORE_COMMON_TEMPLATES_H

namespace {
#include <iostream>
template <typename T> constexpr std::false_type test_stream();

template <typename T>
requires requires(T& t) { std::cout << t; }
constexpr std::true_type test_stream();
} // namespace

namespace uh::cluster {

template <typename func> void foreach (func f) {}

template <typename func, typename head> void foreach (func f, const head& h) {
    f(h);
}

template <typename func, typename head, typename... tail>
void foreach (func f, const head& h, const tail&... t) {
    f(h);
    foreach (f, t...)
        ;
}

template <typename T> struct is_streamable : decltype(test_stream<T>()) {};

template <typename T>
inline constexpr bool is_streamable_v = is_streamable<T>::value;

} // namespace uh::cluster

#endif
