#pragma once

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
    value_guard()
        : m_wrapper()
    {}

    value_guard(value v, fini f)
        : m_wrapper(std::in_place, std::move(v), std::move(f)) {}

    value_guard(value_guard&& vg)
        : m_wrapper(std::move(vg.m_wrapper))
    {
        vg.m_wrapper.reset();
    }

    ~value_guard() {
        try {
            release();
        } catch (...) {
        }
    }

    void release() {
        if (m_wrapper) {
            m_wrapper->f();
            m_wrapper.reset();
        }
    }

    bool empty() const { return !m_wrapper.has_value(); }

    value& operator*() { return m_wrapper->v; }
    value* operator->() { return &m_wrapper->v; }

private:
    struct wrapper {
        value v;
        fini f;
    };

    std::optional<wrapper> m_wrapper;
};

template <typename value, typename fini>
value_guard<value, fini> make_value_guard(value v, fini f) {
    return value_guard<value, fini>(std::move(v), std::move(f));
}

} // namespace uh::cluster
