#include "noop_deduplicator.h"

namespace uh::cluster {

noop_deduplicator::noop_deduplicator(sn::interface& storage)
    : m_storage(storage) {}

coro<dedupe_response> noop_deduplicator::deduplicate(context& ctx,
                                                     std::string_view data) {
    auto addr = co_await m_storage.write(ctx, data, {});

    co_return dedupe_response{.effective_size = data.size(),
                              .addr = std::move(addr)};
}

} // namespace uh::cluster
