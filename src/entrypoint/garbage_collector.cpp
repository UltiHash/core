#include "garbage_collector.h"

#include <common/coroutines/context.h>

namespace uh::cluster::ep {

garbage_collector::garbage_collector(boost::asio::io_context& ctx,
                                     directory& dir, global_data_view& gdv)
    : m_dir(dir),
      m_gdv(gdv) {
    boost::asio::co_spawn(ctx, collect(), boost::asio::detached);
}

coro<void> garbage_collector::collect() {
    boost::asio::steady_timer timer(co_await boost::asio::this_coro::executor);

    context ctx;

    while (true) {
        auto to_delete = co_await m_dir.next_deleted();
        if (!to_delete) {
            co_await m_dir.clear_buckets();
            timer.expires_from_now(POLL_INTERVALL);
            co_await timer.async_wait(boost::asio::use_awaitable);
            continue;
        }

        try {
            auto freed = co_await m_gdv.unlink(ctx, to_delete->addr);
            LOG_DEBUG() << "reclaimed " << freed
                        << " bytes of data by disposing object "
                        << to_delete->id;
        } catch (const std::exception& e) {
            LOG_WARN() << "deleting of object " << to_delete->id
                       << " failed: " << e.what();
        }

        co_await m_dir.remove_object(to_delete->id);
    }
}

} // namespace uh::cluster::ep
