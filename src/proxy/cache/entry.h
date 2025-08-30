#pragma once

#include <chrono>

namespace uh::cluster::proxy::cache {

struct entry_interface {
    using time_point = std::chrono::system_clock::time_point;
    using duration = std::chrono::system_clock::duration;
    entry_interface(const duration& ttl)
        : m_expire_time(std::chrono::system_clock::now() + ttl) {}

    bool is_expired() {
        return std::chrono::system_clock::now() > m_expire_time;
    }

    void touch() {
        m_expire_time = std::chrono::system_clock::now() + duration(0);
    }

    /*
     * For complex eviction policy only. Default eviction doesn't use this.
     */
    const time_point& get_expire_time() const { return m_expire_time; }

private:
    time_point m_expire_time;
};

} // namespace uh::cluster::proxy::cache
