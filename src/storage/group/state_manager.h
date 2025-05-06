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
#include <storage/global/config.h>

namespace uh::cluster::storage {

/*
 * This class manages group state only when it is the leader of the group.
 */
class group_state_manager {
public:
    group_state_manager(boost::asio::io_context& ioc, etcd_manager& etcd,
                        const group_config& config, std::size_t storage_id,
                        const global_data_view_config& global_config)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_group_config{config},
          m_storage_id{storage_id},
          m_global_config{global_config},
          m_group_state_key{
              ns::root.storage_groups[m_group_config.id].group_state},
          m_group_state{group_state::UNDETERMINED},
          m_storage_state_subscriber{m_etcd, m_group_config.id,
                                     m_group_config.storages,
                                     [this]() { manage(); }} {}

    ~group_state_manager() { m_etcd.rm(m_group_state_key); }

private:
    /*
     * Called only on a leader
     */
    void manage() {
        auto storage_states = m_storage_state_subscriber.get();

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

        LOG_DEBUG() << std::format("Group {}: assigned_count {}, has_down {}",
                                   m_group_config.id, assigned_count, has_down);

        if (m_group_state != group_state::HEALTHY and
            assigned_count == m_group_config.storages) {
            m_group_state = group_state::HEALTHY;
            LOG_DEBUG() << "############################## B";
            publish();
        }

        if (m_group_state != group_state::DEGRADED and //
            has_down and assigned_count >= m_group_config.data_shards) {
            m_group_state = group_state::DEGRADED;
            LOG_DEBUG() << "############################## C";
            publish();
        }

        if (m_group_state != group_state::FAILED and //
            assigned_count < m_group_config.data_shards) {
            m_group_state = group_state::FAILED;
            LOG_DEBUG() << "############################## D";
            publish();
        }

        if (m_group_state != group_state::REPAIRING and //
            !has_down and assigned_count >= m_group_config.data_shards and
            assigned_count < m_group_config.storages) {
            m_group_state = group_state::REPAIRING;
            LOG_DEBUG() << "############################## E";
            publish();

            m_repairer.emplace(m_ioc, m_etcd, m_group_config, m_storage_id,
                               m_global_config);
        }
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
    std::string m_group_state_key;
    group_state m_group_state;
    storage_state_subscriber m_storage_state_subscriber;
    std::optional<repairer> m_repairer;
};

} // namespace uh::cluster::storage
