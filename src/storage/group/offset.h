#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
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
        std::size_t max_offset = 0;
        std::string prefix = get_storage_offset_prefix(group_id);
        auto offset_candidates =
            sync_vector_observer<std::optional<std::size_t>>(
                prefix, num_storages, std::nullopt);

        reader r("offset reader", etcd, prefix, {offset_candidates});
        auto candidates = offset_candidates.get();

        auto max_offset_it = std::ranges::max_element(
            candidates,
            []<typename T>(const T& a, const T& b) { return a < b; });

        max_offset = (max_offset_it != candidates.end() ? **max_offset_it : 0);

        return max_offset;
    }
};

} // namespace uh::cluster::storage
