#include "garbage_collector.h"

namespace uh::cluster::ep {

garbage_collector::garbage_collector(boost::asio::io_context& ioc,
                                     directory& dir,
                                     storage::global::global_data_view& gdv)
    : m_stop(),
      m_dir(dir),
      m_gdv(gdv),
      m_stopped(false) {
    boost::asio::co_spawn(ioc, collect().start_trace(), boost::asio::detached);
}

void garbage_collector::stop()
{
    m_stop.request_stop();

    {
        std::unique_lock lock(m_mutex);
        m_condition.wait(lock, [this]{ return m_stopped; });
    }
}

coro<void> garbage_collector::collect() {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);

    while (!m_stop.stop_requested()) {
        auto to_delete = co_await m_dir.next_deleted();
        if (!to_delete) {
            co_await m_dir.clear_buckets();
            timer.expires_after(POLL_INTERVALL);
            co_await timer.async_wait(boost::asio::use_awaitable);
            continue;
        }

        try {
            auto freed = co_await m_gdv.unlink(to_delete->addr);
            LOG_DEBUG() << "reclaimed " << freed
                        << " bytes of data by disposing object "
                        << to_delete->id;
        } catch (const std::exception& e) {
            LOG_WARN() << "deleting of object " << to_delete->id
                       << " failed: " << e.what();
        }

        co_await m_dir.remove_object(to_delete->id);
    }

    {
        std::unique_lock lock(m_mutex);
        m_stopped = true;
    }

    m_condition.notify_all();
}

} // namespace uh::cluster::ep
