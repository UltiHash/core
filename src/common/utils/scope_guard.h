#ifndef COMMON_UTILS_SCOPE_GUARD_H
#define COMMON_UTILS_SCOPE_GUARD_H

namespace uh::cluster {

template <typename fini> class guard {
public:
    guard(fini f)
        : m_f(std::move(f)) {}

    guard(guard&& g)
        : m_active(g.m_active),
          m_f(std::move(g.m_f)) {
        g.m_active = false;
    }

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

/**
 * Guard that can store a value. Use to return a value with RAII
 * cleanup.
 */
template <typename value, typename fini> class value_guard {
public:
    value_guard(value v, fini f)
        : m_value(std::move(v)),
          m_f(std::move(f)) {}

    value_guard(value_guard&& vg)
        : m_active(vg.m_active),
          m_value(std::move(vg.m_value)),
          m_f(std::move(vg.m_f)) {
        vg.m_active = false;
    }

    ~value_guard() {
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

    value& operator*() { return m_value; }
    value* operator->() { return &m_value; }

private:
    bool m_active = true;
    value m_value;
    fini m_f;
};

template <typename value, typename fini>
value_guard<value, fini> make_value_guard(value v, fini f) {
    return value_guard<value, fini>(std::move(v), std::move(f));
}

} // namespace uh::cluster

#endif
