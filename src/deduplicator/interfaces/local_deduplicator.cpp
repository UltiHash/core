#include "local_deduplicator.h"

namespace uh::cluster {

namespace {

template <typename container>
size_t largest_common_prefix(const container& a, const container& b) noexcept {
    auto mismatch = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    return std::distance(a.begin(), mismatch.first);
}

coro<size_t> match_size(context& ctx, global_data_view& storage,
                        std::string_view data, auto frag) {
    if (!frag) {
        co_return 0ull;
    }

    auto& [f, prefix] = *frag;

    std::size_t common = largest_common_prefix(std::string_view(prefix), data);
    if (common < prefix.size()) {
        co_return common;
    }

    auto complete = co_await storage.read(ctx, f.pointer, f.size);

    co_return common +
        largest_common_prefix(data.substr(common),
                              complete.string_view().substr(common));
}

} // namespace

local_deduplicator::local_deduplicator(deduplicator_config config,
                                       global_data_view& storage)
    : m_dedupe_conf(std::move(config)),
      m_fragment_set(m_dedupe_conf.working_dir / "log",
                     m_dedupe_conf.set_capacity, storage),
      m_storage(storage),
      m_dedupe_workers(m_dedupe_conf.worker_thread_count) {}

coro<dedupe_response> local_deduplicator::deduplicate(context& ctx,
                                                      std::string_view data) {
    LOG_DEBUG() << ctx.peer() << ": deduplicate: size=" << data.size();
    fragmentation fragments;
    std::size_t offset = 0;

    while (!data.empty()) {
        auto f = co_await m_dedupe_workers.post_in_workers(
            ctx, [this, &data] { return m_fragment_set.find(data); });

        auto match_low = co_await match_size(ctx, m_storage, data, f.low);
        auto match_high = co_await match_size(ctx, m_storage, data, f.high);

        if (const auto size = std::max(match_low, match_high);
            size > m_dedupe_conf.min_fragment_size) {
            const auto& [frag, prefix] =
                match_low > match_high ? *f.low : *f.high;

            // Add `&& size < data.size()`?
            if (size == m_dedupe_conf.max_fragment_size) {
                offset += co_await pursue_pointer(
                    ctx, data, frag.pointer + m_dedupe_conf.max_fragment_size,
                    (offset == 0), fragments);
            } else {
                fragments.push_stored(frag.pointer, size, data.substr(0, size),
                                      (offset == 0));
                data = data.substr(size);
                offset += size;
            }
        } else {
            auto frag_size =
                std::min(data.size(), m_dedupe_conf.max_fragment_size);

            fragments.push_unstored(data.substr(0, frag_size), (offset == 0),
                                    std::move(f.hint));

            data = data.substr(frag_size);
            offset += frag_size;
        }
    }

    auto stored_fragments = fragments.get_stored_fragments();
    if (!stored_fragments.empty()) {
        auto rejected = co_await m_storage.link(ctx, stored_fragments);

        if (!rejected.empty()) {
            LOG_DEBUG() << ctx.peer() << ": " << rejected.size()
                        << " fragments rejected, " << rejected.data_size()
                        << " in bytes";
            co_await m_dedupe_workers.post_in_workers(ctx, [this, &rejected,
                                                            &fragments] {
                fragments.handle_rejected_fragments(rejected, m_fragment_set);
            });
            LOG_DEBUG() << ctx.peer() << ": handle_rejected_fragments done";
        }
    }

    LOG_DEBUG() << ctx.peer() << ": flushing unstored data to storage";
    co_await fragments.flush_storage(ctx, m_storage);

    LOG_DEBUG() << ctx.peer() << ": flushing fragments to fragment set";
    co_await m_dedupe_workers.post_in_workers(ctx, [this, &fragments] {
        fragments.flush_fragment_set(m_fragment_set);
    });

    LOG_DEBUG() << ctx.peer() << ": creating deduplication response";
    dedupe_response result{.effective_size = fragments.effective_size(),
                           .addr = fragments.make_address()};

    LOG_DEBUG() << ctx.peer() << ": deduplicate finished";
    co_return result;
}

coro<size_t> local_deduplicator::pursue_pointer(context& ctx,
                                                std::string_view& data,
                                                uint128_t pointer, bool header,
                                                fragmentation& fragments) {
    std::size_t common_size;

    uint128_t frag_pointer = pointer - m_dedupe_conf.max_fragment_size;
    std::size_t frag_size = m_dedupe_conf.max_fragment_size;

    do {
        auto stored_data = co_await m_storage.read(ctx, pointer, pursue_size);

        common_size = largest_common_prefix(stored_data.string_view(),
                                            data.substr(frag_size));

        frag_size += common_size;
        pointer += common_size;
    } while (common_size == pursue_size);

    fragments.push_stored(frag_pointer, frag_size, data.substr(0, frag_size),
                          header);

    data = data.substr(frag_size);
    co_return frag_size;
}

} // namespace uh::cluster
