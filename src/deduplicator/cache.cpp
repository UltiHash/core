#include "cache.h"

#include <common/telemetry/metrics.h>

namespace uh::cluster::dd {

cache::cache(global_data_view& gdv, std::size_t capacity)
    : m_gdv(gdv),
      m_lru(capacity) {}

shared_buffer<> cache::read_fragment(const uint128_t& pointer, size_t size) {
    if (size == 0) {
        throw std::runtime_error(
            "read: fragment size must be larger than zero");
    }

    if (auto cp = m_lru.get(pointer); cp && cp->size() >= size) {
        metric<metric_type::gdv_l2_cache_hit_counter>::increase(1);
        return *cp;
    }

    metric<metric_type::gdv_l2_cache_miss_counter>::increase(1);

    auto buffer = m_gdv.read_fragment(pointer, size);
    m_lru.put(pointer, buffer);
    return buffer;
}

coro<shared_buffer<>> cache::read(const uint128_t& pointer, size_t size) {

    if (size == 0) {
        throw std::runtime_error("read: size must be larger than zero");
    }

    if (const auto cp = m_lru.get(pointer); cp && cp->size() >= size) {
        metric<metric_type::gdv_l2_cache_hit_counter>::increase(1);
        co_return *cp;
    }

    metric<metric_type::gdv_l2_cache_miss_counter>::increase(1);

    auto buffer = co_await m_gdv.read(pointer, size);
    m_lru.put(pointer, buffer);
    co_return buffer;
}

} // namespace uh::cluster::dd
