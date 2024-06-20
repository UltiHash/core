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

coro<size_t> match_size(global_data_view& storage, std::string_view data,
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

struct local_deduplicator : public deduplicator_interface {

    local_deduplicator(deduplicator_config config, global_data_view& storage)
        : m_dedupe_conf(std::move(config)),
          m_fragment_set(m_dedupe_conf.working_dir / "log",
                         m_dedupe_conf.set_capacity, storage, false),
          m_storage(storage),
          m_dedupe_workers(m_storage.get_executor(),
                           m_dedupe_conf.worker_thread_count),
          m_dedupe_logger(m_dedupe_conf.working_dir / "dedupe_log", 1000) {}

    coro<dedupe_response> deduplicate(const std::string_view& data) override {
        size_t piece_size = std::ceil(static_cast<double>(data.size()) /
                                      static_cast<double>(pieces_count));
        std::vector<std::string_view> pieces;
        std::vector<std::shared_ptr<awaitable_promise<dedupe_response>>> proms;
        proms.reserve(pieces_count);
        pieces.reserve(pieces_count);
        for (std::size_t i = 0; i < pieces_count; ++i) {
            pieces.emplace_back(data.substr(
                i * piece_size,
                std::min(piece_size, data.size() - i * piece_size)));
            auto p = std::make_shared<awaitable_promise<dedupe_response>>(
                m_storage.get_executor());
            boost::asio::co_spawn(m_storage.get_executor(),
                                  deduplicate_data(pieces.back()),
                                  use_awaitable_promise_cospawn(p));
            proms.emplace_back(p);
        }

        dedupe_response dd_resp;
        for (std::size_t i = 0; i < pieces_count; i++) {
            auto resp = co_await proms[i]->get();
            dd_resp.addr.append_address(resp.addr);
            dd_resp.effective_size += resp.effective_size;
        }
        co_return dd_resp;
    }

private:
    coro<size_t> pursue_pointer(std::string_view& data, uint128_t pointer,
                                fragmentation& fragments) {
        size_t common_size;
        fragment frag{pointer - m_dedupe_conf.max_fragment_size,
                      m_dedupe_conf.max_fragment_size};
        do {
            auto stored_data = co_await m_storage.read(pointer, pursue_size);

            common_size =
                largest_common_prefix(stored_data.string_view(), data);
            data = data.substr(common_size);
            frag.size += common_size;
            pointer += common_size;
        } while (common_size == pursue_size);

        m_dedupe_logger.log_pursue_deduplication(frag.size, frag.pointer);
        fragments.push(frag);
        co_return frag.size;
    }

    coro<dedupe_response> deduplicate_data(std::string_view data) {

        fragmentation fragments(m_dedupe_logger);
        size_t offset = 0;
        size_t non_dedupe_count = 0;
        size_t dedupe_count = 0;

        while (!data.empty()) {
            auto f = co_await m_dedupe_workers.post_in_workers(
                [this, &data] { return m_fragment_set.find(data); });

            auto match_low = co_await match_size(m_storage, data, f.low);
            auto match_high = co_await match_size(m_storage, data, f.high);

            if (const auto size = std::max(match_low, match_high);
                size > m_dedupe_conf.min_fragment_size) {

                const auto& [frag, prefix] =
                    match_low > match_high ? *f.low : *f.high;

                data = data.substr(size);
                if (size == m_dedupe_conf.max_fragment_size) {
                    offset += co_await pursue_pointer(
                        data, frag.pointer + m_dedupe_conf.max_fragment_size,
                        fragments);
                } else {
                    fragments.push(fragment{frag.pointer, size});
                    m_dedupe_logger.log_deduplication(size, prefix,
                                                      frag.pointer, offset);
                    offset += size;
                }
                dedupe_count++;
                continue;
            }

            auto frag_size =
                std::min(data.size(), m_dedupe_conf.max_fragment_size);

            fragments.push(fragmentation::unstored{
                data.substr(0, frag_size), (offset == 0), std::move(f.hint)});

            data = data.substr(frag_size);
            offset += frag_size;
            non_dedupe_count++;
        }
        co_await fragments.flush_data(m_storage);
        co_await m_dedupe_workers.post_in_workers(
            [this, &fragments] { fragments.flush_set(m_fragment_set); });

        dedupe_response result{.effective_size = fragments.effective_size(),
                               .addr = fragments.make_address()};

        if (!result.addr.empty())
            co_await m_storage.sync(result.addr);

        m_dedupe_logger.log_stat(m_fragment_set.size(), dedupe_count,
                                 non_dedupe_count, result.effective_size,
                                 offset);
        co_return result;
    }

    deduplicator_config m_dedupe_conf;
    fragment_set m_fragment_set;
    global_data_view& m_storage;
    worker_pool m_dedupe_workers;
    dedupe_logger m_dedupe_logger;
    constexpr static std::size_t pursue_size = 64 * KIBI_BYTE;
    constexpr static std::size_t pieces_count = 2;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_LOCAL_DEDUPLICATOR_H