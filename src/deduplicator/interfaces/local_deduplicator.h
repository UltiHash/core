
#ifndef UH_CLUSTER_LOCAL_DEDUPLICATOR_H
#define UH_CLUSTER_LOCAL_DEDUPLICATOR_H

#include "common/coroutines/worker_pool.h"
#include "common/global_data/global_data_view.h"

#include "common/coroutines/coro_utils.h"
#include "common/debug/monitor.h"
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
        : m_storage(storage) {
        monitor::get().add_global("pending waits for promise", a);
        monitor::get().add_global("total number of created promises", b);
        monitor::get().add_global("total number of deduplicate calls", c);
        monitor::get().add_global("total number of deduplicate calls", c);
        monitor::get().add_fn("thread count before",
                              [] { return ids1.size(); });
        monitor::get().add_fn("thread count after", [] { return ids2.size(); });
    }

    inline static std::set<std::thread::id> ids1, ids2;

    coro<dedupe_response> deduplicate(const std::string_view& data1) override {
        c++;
        ids1.emplace(std::this_thread::get_id());

        for (int i = 0; i < 100000; ++i) {
            a++;
            b++;
            auto f = std::make_shared<awaitable_promise<int>>(
                m_storage.get_executor());
            f->set(0);
            a += co_await f->get();
            a--;
        }
        ids2.emplace(std::this_thread::get_id());

        dedupe_response result;
        address addr;
        addr.push_fragment({0, 0});
        result.addr = addr;
        co_return result;
    }

private:
    std::atomic<size_t> a = 0, b = 0, c = 0, d = 0, e = 0, g = 0, op[2];
    coro<dedupe_response> deduplicate_data(std::string_view data) {

        c++;
        while (!data.empty()) {
            a++;
            b++;
            auto f = std::make_shared<awaitable_promise<void>>(
                m_storage.get_executor());
            f->set();
            co_await f->get();
            a--;
            data = data.substr(std::min(data.size(), 8 * KIBI_BYTE));
        }
        dedupe_response result;
        address addr;
        addr.push_fragment({0, 0});
        result.addr = addr;
        co_return result;
    }

    // deduplicator_config m_dedupe_conf;
    // fragment_set m_fragment_set;
    global_data_view& m_storage;
    // worker_pool m_dedupe_workers;
    // dedupe_logger m_dedupe_logger;
    constexpr static std::size_t pursue_size = 64 * KIBI_BYTE;
    constexpr static std::size_t pieces_count = 1;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_LOCAL_DEDUPLICATOR_H
