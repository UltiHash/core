#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>

namespace uh::cluster::storage {

using offset_t = std::size_t;

class offset_manager {
public:
    using callback_t = subscriber::callback_t;
    offset_manager(etcd_manager& etcd, std::size_t group_id,
                   std::size_t num_storages)
        : m_etcd{etcd},
          m_prefix{get_storage_offset_prefix(group_id)},
          future{promise.get_future()},
          m_offset_candidates{m_prefix, num_storages, 0},
          m_reader{"offset_manager", etcd, m_prefix, {m_offset_candidates}} {}

    ~offset_manager() { m_etcd.rm(m_prefix); }

    static void put(etcd_manager& etcd, std::size_t group_id,
                    std::size_t storage_id, offset_t val) {
        etcd.put(get_storage_offset_prefix(group_id)[storage_id],
                 serialize(val));
    }

    auto summarize_offsets(std::chrono::seconds timeout = 5s) {
        future.wait_for(timeout);
        auto candidates = m_offset_candidates.get();
        auto max_offset = std::ranges::max_element(
            candidates,
            []<typename T>(const T& a, const T& b) { return *a < *b; });

        return (max_offset != candidates.end() ? **max_offset : 0);
    }

private:
    etcd_manager& m_etcd;
    offset_prefix_t m_prefix;
    std::promise<void> promise;
    std::future<void> future;
    vector_observer<offset_t> m_offset_candidates;
    reader m_reader;
};

} // namespace uh::cluster::storage
