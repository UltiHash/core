#ifndef CORE_COMMON_TEMPLATES_H
#define CORE_COMMON_TEMPLATES_H

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

} // namespace uh::cluster

#endif
