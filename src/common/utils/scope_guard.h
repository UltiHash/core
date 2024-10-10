#ifndef COMMON_UTILS_SCOPE_GUARD_H
#define COMMON_UTILS_SCOPE_GUARD_H

namespace uh::cluster {

template <typename fini> class guard {
public:
    guard(fini f)
        : m_f(std::move(f)) {}

    ~guard() {
        try {
            m_f();
        } catch (...) {
        }
    }

private:
    fini m_f;
};

template <typename fini> guard<fini> scope_guard(fini f) {
    return guard<fini>(std::move(f));
}

} // namespace uh::cluster

#endif
