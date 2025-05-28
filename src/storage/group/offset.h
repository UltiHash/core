#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <future>
#include <storage/group/state.h>

namespace uh::cluster::storage {

class offset_manager {
public:
    using callback_t = subscriber::callback_t;
    offset_manager() = delete;

    static void rm(etcd_manager& etcd, std::size_t group_id,
                   std::size_t storage_id) {
        etcd.rm(get_storage_offset_prefix(group_id)[storage_id]);
    }
    static void put(etcd_manager& etcd, std::size_t group_id,
                    std::size_t storage_id, std::size_t val) {
        etcd.put(get_storage_offset_prefix(group_id)[storage_id],
                 serialize(val));
    }

    static auto summarize_offsets(etcd_manager& etcd, std::size_t group_id,
                                  std::size_t num_storages) {
        std::string prefix = get_storage_offset_prefix(group_id);
        auto offset_observer = sync_vector_observer<std::optional<std::size_t>>(
            prefix, num_storages, std::nullopt);

        auto start = std::chrono::steady_clock::now();
        do {
            reader r("", etcd, prefix, {offset_observer});
            auto candidates = offset_observer.get();

            bool all_have_value = std::ranges::all_of(
                candidates, [](const auto& opt) { return opt.has_value(); });
            if (all_have_value) {
                break;
            }
            if ((std::chrono::steady_clock::now() - start) >=
                (OFFSET_GATHERING_TIMEOUT)) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        } while (true);

        auto offsets = offset_observer.get();

        auto max_offset_it = std::ranges::max_element(
            offsets, []<typename T>(const T& a, const T& b) { return a < b; });

        if (max_offset_it != offsets.end() && max_offset_it->has_value()) {
            return max_offset_it->value();
        } else {
            throw std::runtime_error("All elements are std::nullopt");
        }
    }
};

} // namespace uh::cluster::storage
