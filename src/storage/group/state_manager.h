#pragma once

#include "config.h"
#include "externals.h"
#include "internals.h"
#include "offset.h"
#include "repairer.h"

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
          m_config{group_config::create(
              m_etcd.get(ns::root.storage_groups.group_configs[group_id]))},
          m_num_storages{m_config.data_shards + m_config.parity_shards},
          m_storage_id{storage_id},
          m_offset{
              state_manager::summarize_offsets(etcd, group_id, m_storage_id)},
          m_externals_publisher(etcd, group_id, storage_id),
          m_group_state{group_state::INITIALIZING},
          m_internals_subscriber{m_etcd, m_group_id, m_num_storages,
                                 [this](etcd_manager::response) { manage(); }} {
        // TODO: set real offset using m_offset
        m_externals_publisher.put_group_state(m_group_state);
    }

private:
    static std::size_t summarize_offsets(etcd_manager& etcd,
                                         std::size_t group_id,
                                         std::size_t num_storages) {
        auto os = offset_subscriber(etcd, group_id, num_storages);
        auto offsets = os.get();

        auto max_offset = std::ranges::max_element(
            offsets,
            []<typename T>(const T& a, const T& b) { return *a < *b; });

        return max_offset != offsets.end() ? **max_offset : 0;
    }

    /*
     * Called only on a leader
     */
    void manage() {
        auto group_initialized = m_internals_subscriber.get_group_initialized();
        auto storage_states = m_internals_subscriber.get_storage_states();

        bool has_down = false;
        auto assigned_count = 0ul;
        for (const auto& val : storage_states) {
            switch (*val) {
            case storage_state::DOWN:
                has_down = true;
                break;
            case storage_state::ASSIGNED:
                ++assigned_count;
                break;
            default:
                break;
            }
        }

        // TODO: manage state here
        if (m_group_state == group_state::INITIALIZING) {
            if (*group_initialized == false) {

                for (const auto& val : storage_states) {
                    if (*val == storage_state::NEW) {
                        // TODO: Set storage_state to ASSIGNED using
                        // STORAGE_UPDATE_STATE_REQ.
                        // To do so, we should know hostports of the storages
                        // and have their remote interfaces temporarily.
                    }
                    internals_publisher::set_group_initialized(m_etcd,
                                                               m_group_id);
                    m_group_state = group_state::HEALTHY;
                    m_externals_publisher.put_group_state(m_group_state);
                }
            }

            if (has_down)
                return;
        }

        if (m_group_state != group_state::HEALTHY and
            assigned_count == m_num_storages) {
            m_group_state = group_state::HEALTHY;
            m_externals_publisher.put_group_state(m_group_state);
        }

        if (m_group_state != group_state::DEGRADED and //
            has_down and assigned_count >= m_config.data_shards) {
            m_group_state = group_state::DEGRADED;
            m_externals_publisher.put_group_state(m_group_state);
        }

        if (m_group_state != group_state::FAILED and //
            assigned_count < m_config.data_shards) {
            m_group_state = group_state::FAILED;
            m_externals_publisher.put_group_state(m_group_state);
        }

        if (m_group_state != group_state::REPAIRING and //
            !has_down and assigned_count >= m_config.data_shards and
            assigned_count < m_num_storages) {
            m_group_state = group_state::REPAIRING;
            m_externals_publisher.put_group_state(m_group_state);

            auto reader =
                externals_subscriber(m_etcd, m_group_id, m_num_storages);
            m_repairer.emplace(storage_states, reader.get_storage_hostports());
        } else if (m_group_state == group_state::REPAIRING and
                   assigned_count == m_num_storages) {
            if (m_repairer->is_changed(storage_states)) {
                m_repairer.reset();
                auto reader =
                    externals_subscriber(m_etcd, m_group_id, m_num_storages);
                m_repairer.emplace(storage_states,
                                   reader.get_storage_hostports());
            }
        }
    }

    etcd_manager& m_etcd;
    std::size_t m_group_id;
    group_config m_config;
    std::size_t m_num_storages;
    std::size_t m_storage_id;
    std::size_t m_offset;
    externals_publisher m_externals_publisher;
    group_state m_group_state;
    internals_subscriber m_internals_subscriber;
    std::optional<repairer> m_repairer;
};

} // namespace uh::cluster::storage
