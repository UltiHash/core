#include "local_deduplicator.h"

namespace uh::cluster {

namespace {

template <typename container>
size_t largest_common_prefix(const container& a, const container& b) noexcept {
    auto mismatch = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    return std::distance(a.begin(), mismatch.first);
}

coro<size_t> match_size(storage::global::cache& storage, std::string_view data,
                        auto frag) {
    if (!frag) {
        co_return 0ull;
    }

    auto& [f, prefix] = *frag;

    std::size_t common = largest_common_prefix(std::string_view(prefix), data);
    if (common < prefix.size()) {
        co_return common;
    }

    auto complete = co_await storage.read(f.pointer, f.size);

    co_return common +
        largest_common_prefix(data.substr(common),
                              complete.string_view().substr(common));
}

} // namespace

local_deduplicator::local_deduplicator(deduplicator_config config,
                                       storage::data_view& storage,
                                       storage::global::cache& cache)
    : m_dedupe_conf(std::move(config)),
      m_storage(storage),
      m_cache(cache),
      m_fragment_set(m_dedupe_conf.set_capacity, m_cache),
      m_dedupe_workers(m_dedupe_conf.worker_thread_count) {}

coro<dedupe_response> local_deduplicator::deduplicate(std::string_view data) {
    auto span = co_await boost::asio::this_coro::span;

    auto peer =
        boost::asio::context::get_pointer<boost::asio::ip::tcp::endpoint>(
            span->context(), "peer");

    span->set_attribute("data-size", data.size());

    fragmentation fragments;
    std::size_t offset = 0;
    span->add_event("deduplicator-fragment-search");
    {
        while (!data.empty()) {
            auto f = co_await m_dedupe_workers.post_in_workers(
                [this, &data] { return m_fragment_set.find(data); });

            auto match_low = co_await match_size(m_cache, data, f.low);
            auto match_high = co_await match_size(m_cache, data, f.high);

            if (auto size = std::max(match_low, match_high);
                size > m_dedupe_conf.min_fragment_size) {
                const auto& [frag, prefix] =
                    match_low > match_high ? *f.low : *f.high;

                fragments.push_stored(frag.pointer, size,
                                      data.substr(0, size), (offset == 0));
                data = data.substr(size);
                offset += size;
            } else {
                auto frag_size =
                    std::min(data.size(), m_dedupe_conf.max_fragment_size);

                fragments.push_unstored(data.substr(0, frag_size),
                                        (offset == 0), std::move(f.hint));

                data = data.substr(frag_size);
                offset += frag_size;
            }
        }
    }

    span->add_event("deduplicator-fragment-link");
    {
        auto stored_fragments = fragments.get_stored_fragments();
        if (!stored_fragments.empty()) {
            auto rejected = co_await m_storage.link(stored_fragments);

            if (!rejected.empty()) {
                LOG_DEBUG() << *peer << ": " << rejected.size()
                            << " fragments rejected, " << rejected.data_size()
                            << " in bytes";
                co_await m_dedupe_workers.post_in_workers(
                    [this, &rejected, &fragments] {
                        fragments.handle_rejected_fragments(rejected,
                                                            m_fragment_set);
                    });
                LOG_DEBUG() << *peer << ": "
                            << "handle_rejected_fragments done";
            }
        }
    }

    span->add_event("deduplicator-flush-storage");
    {
        LOG_DEBUG() << *peer << ": "
                    << "flushing unstored data to storage";
        co_await fragments.flush_storage(m_storage);
    }

    span->add_event("deduplicator-flush-fragments");
    {
        LOG_DEBUG() << *peer << ": "
                    << "flushing fragments to fragment set";
        co_await m_dedupe_workers.post_in_workers([this, &fragments] {
            fragments.flush_fragment_set(m_fragment_set);
        });
    }

    LOG_DEBUG() << *peer << ": "
                << "creating deduplication response";
    dedupe_response result{.effective_size = fragments.effective_size(),
                           .addr = fragments.make_address()};

    LOG_DEBUG() << *peer << ": "
                << "deduplicate finished: " << result.effective_size
                << " effective bytes, " << result.addr.data_size()
                << " raw bytes, " << result.addr.size() << " fragments";
    co_return result;
}

} // namespace uh::cluster
