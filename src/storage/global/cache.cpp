#include "cache.h"

#include <common/telemetry/log.h>
#include <common/telemetry/metrics.h>

namespace uh::cluster::storage::global {

cache::cache(boost::asio::io_context& ioc, data_view& storage,
             std::size_t capacity)
    : m_ioc(ioc),
      m_storage(storage),
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
    auto context = THREAD_LOCAL_CONTEXT;

    if (boost::asio::trace_span::enable &&
        !boost::asio::trace_span::check_context(context)) {
        LOG_ERROR() << "[read_fragment] The context to be "
                       "encoded is invalid";
    }

    auto buffer =
        boost::asio::co_spawn(
            m_ioc, m_storage.read(pointer, size).continue_trace(context),
            boost::asio::use_future)
            .get();
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

    auto buffer = co_await m_storage.read(pointer, size);
    m_lru.put(pointer, buffer);
    co_return buffer;
}

} // namespace uh::cluster::storage::global
