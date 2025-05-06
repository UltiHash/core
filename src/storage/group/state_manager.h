#pragma once

#include "config.h"
#include "externals.h"
#include "internals.h"
#include "offset.h"
#include "repairer.h"

#include <algorithm>
#include <common/etcd/candidate.h>
#include <common/etcd/namespace.h>
#include <common/etcd/registry/storage_registry.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <storage/global/config.h>

namespace uh::cluster::storage {

/*
 * This class manages group state only when it is the leader of the group.
 */
class group_state_manager {
public:
    group_state_manager(boost::asio::io_context& ioc, etcd_manager& etcd,
                        const group_config& config, std::size_t storage_id,
                        const global_data_view_config& global_config,
                        storage_registry& storage_registry)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_group_config{config},
          m_storage_id{storage_id},
          m_global_config{global_config},
          m_storage_registry{storage_registry},

          m_group_initialized{
              group_initialized::get(m_etcd, m_group_config.id)},
          m_group_state{group_state::UNDETERMINED},

          m_group_state_key{
              ns::root.storage_groups[m_group_config.id].group_state},
          m_storage_state_subscriber{m_etcd, m_group_config.id,
                                     m_group_config.storages,
                                     [this]() { manage(); }} {
        // TODO: After timeout, trigger manage function with setting
        // group_initialized.
        LOG_DEBUG() << std::format(
            "Group {}'s state manager (storage {}) is constructed",
            m_group_config.id, m_storage_id);
    }

    ~group_state_manager() {
        LOG_DEBUG() << std::format(
            "Group {}'s state manager (storage {}) is destructed",
            m_group_config.id, m_storage_id);
        m_etcd.rm(m_group_state_key);
    }

private:
    struct statistics {
        bool has_down = false;
        std::size_t assigned_count = 0ul;
    };

    /*
     * Called only on a leader
     */
    void manage() {
        auto storage_states = m_storage_state_subscriber.get();
        auto stats = get_statistics(storage_states);
        LOG_DEBUG() << std::format("Group {}: assigned_count {}, has_down {}",
                                   m_group_config.id, stats.assigned_count,
                                   stats.has_down);

        if (not m_group_initialized) {
            LOG_INFO() << "Group isn't initialized";

            if (stats.has_down)
                return;

            for (auto i = 0ul; i < storage_states.size(); ++i) {
                if (*storage_states[i] == storage_state::NEW) {
                    if (i == m_storage_id) {
                        m_storage_registry.set(storage_state::ASSIGNED);
                        m_storage_registry.publish();
                    } else {
                        m_storage_registry.set_others_persistant(
                            i, storage_state::ASSIGNED);
                    }
                }
            }
            if (stats.assigned_count != m_group_config.storages)
                return;

            m_group_initialized = true;
            group_initialized::put(m_etcd, m_group_config.id,
                                   m_group_initialized);
        }

        if (m_group_state != group_state::HEALTHY and
            stats.assigned_count == m_group_config.storages) {
            m_group_state = group_state::HEALTHY;
            publish();
        }

        if (m_group_state != group_state::DEGRADED and //
            stats.has_down and
            stats.assigned_count >= m_group_config.data_shards) {
            m_group_state = group_state::DEGRADED;
            publish();
        }

        if (m_group_state != group_state::FAILED and //
            stats.assigned_count < m_group_config.data_shards) {
            m_group_state = group_state::FAILED;
            publish();
        }

        if (m_group_state != group_state::REPAIRING and //
            !stats.has_down and
            stats.assigned_count >= m_group_config.data_shards and
            stats.assigned_count < m_group_config.storages) {
            m_group_state = group_state::REPAIRING;
            publish();

            m_repairer.emplace(m_ioc, m_etcd, m_group_config, m_storage_id,
                               m_global_config);
        }
    }

    statistics get_statistics(
        std::vector<std::shared_ptr<storage_state>>& storage_states) {

        statistics rv;
        for (const auto& val : storage_states) {
            LOG_DEBUG() << "storage state: " << magic_enum::enum_name(*val);
            switch (*val) {
            case storage_state::DOWN:
                rv.has_down = true;
                break;
            case storage_state::ASSIGNED:
                rv.assigned_count++;
                break;
            default:
                break;
            }
        }
        return rv;
    }

    void publish() {
        LOG_DEBUG() << std::format("Group {}: publish group state {}",
                                   m_group_config.id,
                                   magic_enum::enum_name(m_group_state));
        m_etcd.put(m_group_state_key, serialize(m_group_state));
    }

    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    group_config m_group_config;
    std::size_t m_storage_id;
    global_data_view_config m_global_config;
    storage_registry& m_storage_registry;

    bool m_group_initialized;
    group_state m_group_state;

    std::string m_group_state_key;
    storage_state_subscriber m_storage_state_subscriber;
    std::optional<repairer> m_repairer;
};

} // namespace uh::cluster::storage
