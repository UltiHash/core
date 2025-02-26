#include "cache.h"

#include <common/telemetry/metrics.h>

namespace uh::cluster::dd {

cache::cache(boost::asio::io_context& ioc, sn::interface& gdv,
             std::size_t capacity)
    : m_ioc(ioc),
      m_gdv(gdv),
      m_lru(capacity) {}

shared_buffer<> cache::read(context& ctx, const uint128_t& pointer,
                            size_t size) {
    if (size == 0) {
        throw std::runtime_error("Read fragment size must be larger than zero");
    }

    if (auto cp = m_lru.get(pointer); cp && cp->size() >= size) {
        metric<metric_type::gdv_l2_cache_hit_counter>::increase(1);
        return *cp;
    }

    metric<metric_type::gdv_l2_cache_miss_counter>::increase(1);

    shared_buffer<char> buffer(size);
    fragment f{pointer, size};
    address addr(f);
    auto future = boost::asio::co_spawn(
        m_ioc, m_gdv.read(ctx, addr, buffer), boost::asio::use_future);

    auto count = future.get();
    buffer.resize(count);

    m_lru.put(pointer, buffer);
    return buffer;
}

coro<shared_buffer<>> cache::read(context& ctx, const uint128_t& pointer,
                                  size_t size, cache::use_coro) {

    (void)m_ioc;

    if (size == 0) {
        throw std::runtime_error("Read size must be larger than zero");
    }

    if (const auto cp = m_lru.get(pointer); cp && cp->size() >= size) {
        metric<metric_type::gdv_l2_cache_hit_counter>::increase(1);
        co_return *cp;
    }

    metric<metric_type::gdv_l2_cache_miss_counter>::increase(1);

    shared_buffer<char> buffer(size);
    auto count =
        co_await m_gdv.read(ctx, fragment{pointer, size}, buffer.span());
    buffer.resize(count);
    m_lru.put(pointer, buffer);
    co_return buffer;
}

} // namespace uh::cluster::dd
