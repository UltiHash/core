#pragma once

#include "config.h"
#include "externals.h"
#include "internals.h"
#include "offset.h"

#include <algorithm>
#include <common/etcd/candidate.h>
#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>

namespace uh::cluster::storage {

/*
 * This class manages group state only when it is the leader of the group.
 */
class state_manager {
public:
    state_manager(etcd_manager& etcd, std::size_t group_id,
                  std::size_t storage_id)
        : m_etcd{etcd},
          m_group_id{group_id},
          m_group_config{group_config::create(
              m_etcd.get(ns::root.storage_groups.group_configs[group_id]))},
          m_num_storages{m_group_config.data_shards +
                         m_group_config.parity_shards},
          m_storage_id{storage_id},
          m_offset{
              state_manager::summarize_offsets(etcd, group_id, m_storage_id)},
          m_externals_publisher(etcd, group_id, storage_id),
          m_group_state{group_state::INITIALIZING},
          m_internals_subscriber{m_etcd, m_group_id, m_num_storages,
                                 [this](etcd_manager::response) { manage(); }} {
        // TODO: set real offset using m_offset
    }

private:
    static std::size_t summarize_offsets(etcd_manager& etcd, size_t group_id,
                                         size_t num_storages) {
        auto offsets = offset_subscriber(etcd, group_id, num_storages).get();

        auto max_offset = std::ranges::max_element(
            offsets,
            []<typename T>(const T& a, const T& b) { return *a < *b; });

        return max_offset != offsets.end() ? **max_offset : 0;
    }
    /*
     * Called only on a leader
     */
    void manage() {
        // auto group_initialized =
        // m_internals_subscriber.get_group_initialized(); auto storage_states =
        // m_internals_subscriber.get_storage_states();

        // TODO: manage state here
        m_group_state = group_state::INITIALIZING;
        // m_externals_publisher.put_group_state(*m_group_state);
    }

    etcd_manager& m_etcd;
    std::size_t m_group_id;
    group_config m_group_config;
    std::size_t m_num_storages;
    std::size_t m_storage_id;
    std::size_t m_offset;
    externals_publisher m_externals_publisher;

    /*
     * Used only on a leader
     */
    group_state m_group_state;
    internals_subscriber m_internals_subscriber;
};

} // namespace uh::cluster::storage
