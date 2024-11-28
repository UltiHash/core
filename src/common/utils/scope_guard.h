#ifndef COMMON_UTILS_SCOPE_GUARD_H
#define COMMON_UTILS_SCOPE_GUARD_H

#include <utility>

namespace uh::cluster {

template <typename fini> class guard {
public:
    guard(fini f)
        : m_f(std::move(f)) {}

    ~guard() {
        try {
            release();
        } catch (...) {
        }
    }

    void release() {
        if (m_active) {
            m_active = false;
            m_f();
        }
    }

private:
    bool m_active = true;
    fini m_f;
};

template <typename fini> guard<fini> scope_guard(fini f) {
    return guard<fini>(std::move(f));
}

} // namespace uh::cluster

#endif
