
#ifndef UH_CLUSTER_LOCAL_DEDUPLICATOR_H
#define UH_CLUSTER_LOCAL_DEDUPLICATOR_H

#include "common/coroutines/worker_pool.h"
#include "common/global_data/global_data_view.h"
#include "config.h"
#include "deduplicator/dedupe_set/fragment_set.h"
#include "deduplicator/fragmentation.h"
#include "deduplicator_interface.h"

namespace uh::cluster {

namespace {

template <typename container>
size_t largest_common_prefix(const container& a, const container& b) noexcept {
    auto mismatch = std::mismatch(a.begin(), a.end(), b.begin(), b.end());
    return std::distance(a.begin(), mismatch.first);
}

size_t match_size(global_data_view& storage, std::string_view data, auto frag) {
    if (!frag) {
        return 0ull;
    }

    const fragment_set_element& f = *frag;

    std::size_t common =
        largest_common_prefix(std::string_view(f.prefix()), data);
    if (common < f.prefix().size()) {
        return common;
    }

    auto complete = storage.read_fragment(f.pointer(), f.size());
    return largest_common_prefix(data.substr(common),
                                 complete.string_view().substr(common));
}

} // namespace

struct local_deduplicator : public deduplicator_interface {

    local_deduplicator(deduplicator_config config, global_data_view& storage)
        : m_dedupe_conf(std::move(config)),
          m_fragment_set(m_dedupe_conf.working_dir / "log", storage, false),
          m_storage(storage),
          m_dedupe_workers(m_storage.get_executor(),
                           config.worker_thread_count),
          m_fragment_buffer_size(config.fragment_buffer_size) {}

    coro<dedupe_response> deduplicate(const std::string_view& data) override {
        const std::size_t pieces_count =
            std::min(m_storage.get_storage_service_connection_count(),
                     static_cast<std::size_t>(std::ceil(
                         static_cast<double>(data.size()) /
                         static_cast<double>(
                             m_dedupe_conf.dedupe_worker_minimum_data_size))));
        size_t piece_size = std::ceil(static_cast<double>(data.size()) /
                                      static_cast<double>(pieces_count));
        std::vector<std::string_view> pieces;
        pieces.reserve(pieces_count);
        for (std::size_t i = 0; i < pieces_count; ++i) {
            pieces.emplace_back(data.substr(
                i * piece_size,
                std::min(piece_size, data.size() - i * piece_size)));
        }

        auto responses =
            co_await m_dedupe_workers.broadcast_from_io_thread_in_workers(
                [this](const auto& piece) { return deduplicate_data(piece); },
                pieces);

        for (std::size_t i = 1; i < pieces_count; i++) {
            responses[0].addr.append_address(responses[i].addr);
            responses[0].effective_size += responses[i].effective_size;
        }
        co_await m_dedupe_workers.post_in_workers(
            [this, &responses]() { m_storage.sync(responses[0].addr); });
        co_return responses[0];
    }

private:
    dedupe_response deduplicate_data(std::string_view data) {

        fragmentation fragments;

        while (!data.empty()) {
            const auto f = m_fragment_set.find(data);

            auto match_low = match_size(m_storage, data, f.low);
            auto match_high = match_size(m_storage, data, f.high);

            if (std::max(match_low, match_high) >
                m_dedupe_conf.min_fragment_size) {

                const fragment_set_element& element =
                    match_low > match_high ? *f.low : *f.high;
                std::size_t size =
                    match_low > match_high ? match_low : match_high;

                fragments.push(fragment{element.pointer(), size});
                data = data.substr(size);

                continue;
            }

            auto frag_size =
                std::min(data.size(), m_dedupe_conf.max_fragment_size);
            fragments.push(
                fragmentation::unstored{data.substr(0, frag_size), f.hint});
            data = data.substr(frag_size);

            if (fragments.unstored_size() >= m_fragment_buffer_size) {
                fragments.flush(m_storage, m_fragment_set);
            }
        }

        fragments.flush(m_storage, m_fragment_set);
        dedupe_response result{.effective_size = fragments.effective_size(),
                               .addr = fragments.make_address()};

        return result;
    }

    deduplicator_config m_dedupe_conf;
    fragment_set m_fragment_set;
    global_data_view& m_storage;
    worker_pool m_dedupe_workers;
    std::size_t m_fragment_buffer_size = 8 * MEBI_BYTE;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_LOCAL_DEDUPLICATOR_H
