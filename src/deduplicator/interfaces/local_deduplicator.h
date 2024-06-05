
#ifndef UH_CLUSTER_LOCAL_DEDUPLICATOR_H
#define UH_CLUSTER_LOCAL_DEDUPLICATOR_H

#include "common/coroutines/worker_pool.h"
#include "common/global_data/global_data_view.h"

#include "deduplicator/dedupe_logger.h"
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

    auto& [f, prefix] = *frag;

    std::size_t common = largest_common_prefix(std::string_view(prefix), data);
    if (common < prefix.size()) {
        return common;
    }

    auto complete = storage.read_fragment(f.pointer, f.size);

    return common +
           largest_common_prefix(data.substr(common),
                                 complete.string_view().substr(common));
}

} // namespace

struct local_deduplicator : public deduplicator_interface {

    local_deduplicator(deduplicator_config config, global_data_view& storage)
        : m_dedupe_conf(std::move(config)),
          m_fragment_set(m_dedupe_conf.working_dir / "log",
                         m_dedupe_conf.set_capacity, storage, false),
          m_storage(storage),
          m_dedupe_workers(m_storage.get_executor(),
                           m_dedupe_conf.worker_thread_count),
          m_dedupe_logger(m_dedupe_conf.working_dir / "dedupe_log", 1000),
          m_fragment_buffer_size(m_dedupe_conf.fragment_buffer_size) {}

    coro<dedupe_response> deduplicate(const std::string_view& data) override {
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
    void pursue_pointer(std::string_view& data, uint128_t pointer,
                        fragmentation& fragments) {
        size_t common_size;
        size_t deduplicated = 0;
        shared_buffer<char> stored_data;
        do {
            try {
                stored_data = m_storage.read_fragment(
                    pointer, m_dedupe_conf.max_fragment_size * pursue_count);

            } catch (const std::exception&) {
                break;
            }
            common_size =
                largest_common_prefix(stored_data.string_view(), data);
            if (common_size > m_dedupe_conf.min_fragment_size) {
                fragments.push(fragment{pointer, common_size});
                data = data.substr(common_size);
                pointer += common_size;
                deduplicated += common_size;
            }
        } while (common_size == m_dedupe_conf.max_fragment_size * pursue_count);
        m_dedupe_logger.log_pursue_deduplication(deduplicated,
                                                 pointer - deduplicated);
    }

    dedupe_response deduplicate_data(std::string_view data) {

        fragmentation fragments(m_dedupe_logger);
        size_t offset = 0;
        size_t non_dedupe_count = 0;
        size_t dedupe_count = 0;

        while (!data.empty()) {
            const auto f = m_fragment_set.find(data);

            auto match_low = match_size(m_storage, data, f.low);
            auto match_high = match_size(m_storage, data, f.high);

            if (const auto size = std::max(match_low, match_high);
                size > m_dedupe_conf.min_fragment_size) {

                const auto& [frag, prefix] =
                    match_low > match_high ? *f.low : *f.high;

                fragments.push(fragment{frag.pointer, size});
                m_fragment_set.mark_deduplication(frag, f.hint);
                m_dedupe_logger.log_deduplication(size, prefix, frag.pointer,
                                                  offset);

                data = data.substr(size);
                if (size == m_dedupe_conf.max_fragment_size) {
                    pursue_pointer(
                        data, frag.pointer + m_dedupe_conf.max_fragment_size,
                        fragments);
                }
                offset += size;
                dedupe_count++;
                continue;
            }

            auto frag_size =
                std::min(data.size(), m_dedupe_conf.max_fragment_size);
            fragments.push(
                fragmentation::unstored{data.substr(0, frag_size), f.hint});
            data = data.substr(frag_size);
            offset += frag_size;
            non_dedupe_count++;
            if (fragments.unstored_size() >= m_fragment_buffer_size) {
                fragments.flush(m_storage, m_fragment_set);
            }
        }

        fragments.flush(m_storage, m_fragment_set);
        dedupe_response result{.effective_size = fragments.effective_size(),
                               .addr = fragments.make_address()};
        m_dedupe_logger.log_stat(m_fragment_set.size(), dedupe_count,
                                 non_dedupe_count, result.effective_size,
                                 offset);

        return result;
    }

    deduplicator_config m_dedupe_conf;
    fragment_set m_fragment_set;
    global_data_view& m_storage;
    worker_pool m_dedupe_workers;
    dedupe_logger m_dedupe_logger;
    const std::size_t m_fragment_buffer_size;
    constexpr static std::size_t pursue_count = 4;
    constexpr static std::size_t pieces_count = 2;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_LOCAL_DEDUPLICATOR_H
