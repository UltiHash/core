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
class state_manager {
public:
    state_manager(boost::asio::io_context& ioc, etcd_manager& etcd,
                  const group_config& config, std::size_t storage_id,
                  const global_data_view_config& global_config)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_group_config{config},
          m_num_storages{m_group_config.data_shards +
                         m_group_config.parity_shards},
          m_global_config{global_config},
          m_group_state_key{
              ns::root.storage_groups[m_group_config.id].group_state},
          m_group_state{group_state::UNDETERMINED},
          m_storage_state_subscriber{
              m_etcd, m_group_config.id, m_num_storages,
              [this](std::size_t id, storage_state state) { manage(); }} {

        publish(m_group_state);
    }
    ~state_manager() { m_etcd.rm(m_group_state_key); }

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

        if (m_group_state != group_state::HEALTHY and
            assigned_count == m_num_storages) {
            m_group_state = group_state::HEALTHY;
            publish(m_group_state);
        }

        if (m_group_state != group_state::DEGRADED and //
            has_down and assigned_count >= m_group_config.data_shards) {
            m_group_state = group_state::DEGRADED;
            publish(m_group_state);
        }

        if (m_group_state != group_state::FAILED and //
            assigned_count < m_group_config.data_shards) {
            m_group_state = group_state::FAILED;
            publish(m_group_state);
        }

        if (m_group_state != group_state::REPAIRING and //
            !has_down and assigned_count >= m_group_config.data_shards and
            assigned_count < m_num_storages) {
            m_group_state = group_state::REPAIRING;
            publish(m_group_state);

            auto reader = externals_subscriber(
                m_etcd, m_group_config.id, m_num_storages,
                service_factory<storage_interface>(
                    m_ioc, m_global_config.storage_service_connection_count));
            m_repairer.emplace(storage_states, reader.get_storage_services());
        }
    }

    void publish(group_state value) {
        m_etcd.put(m_group_state_key, serialize(value));
    }
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    group_config m_group_config;
    std::size_t m_num_storages;
    global_data_view_config m_global_config;
    std::string m_group_state_key;
    group_state m_group_state;
    storage_state_subscriber m_storage_state_subscriber;
    std::optional<repairer> m_repairer;
};

} // namespace uh::cluster::storage
