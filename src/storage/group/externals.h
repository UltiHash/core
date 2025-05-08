#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber.h>

#include <common/etcd/candidate.h>
#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/hostport.h>
#include <common/utils/strings.h>
#include <storage/group/state.h>

#include <common/etcd/service_discovery/storage_index.h>

namespace uh::cluster::storage {

class group_state_manager {
public:
    group_state_manager(etcd_manager& etcd, std::size_t group_id)
        : m_etcd{etcd},
          m_key{get_prefix(group_id).group_state},
          m_group_state{group_state::UNDETERMINED} {}

    void put(group_state state) {
        m_group_state = state;
        m_etcd.put(m_key, serialize(state));
    }
    group_state get() { return m_group_state; }

private:
    etcd_manager& m_etcd;
    std::string m_key;
    group_state m_group_state;
};

/*
 * Group-wise subscriber
 */
class externals_subscriber {
public:
    using callback_t = subscriber::callback_t;
    externals_subscriber(etcd_manager& etcd, std::size_t group_id,
                         std::size_t num_storages,
                         service_factory<storage_interface> service_factory,
                         callback_t callback = nullptr)
        : m_prefix{get_prefix(group_id)},
          m_leader{m_prefix.leader, candidate_observer::staging_id},
          m_group_state{m_prefix.group_state},
          m_storage_states{m_prefix.storage_states, num_storages},
          m_storage_index{num_storages},
          m_storage_hostports{m_prefix.storage_hostports,
                              std::move(service_factory),
                              {m_storage_index}},
          m_subscriber{
              "externals_subscriber",
              etcd,
              m_prefix,
              {m_leader, m_group_state, m_storage_states, m_storage_hostports},
              std::move(callback)} {}

    auto get_leader() { return m_leader.get(); };
    auto get_group_state() { return m_group_state.get(); };
    auto get_storage_states() { return m_storage_states.get(); };
    auto get_storage_services() { return m_storage_index.get(); };

private:
    prefix_t m_prefix;
    value_observer<candidate::id_t> m_leader;
    value_observer<group_state> m_group_state;
    vector_observer<storage_state> m_storage_states;

    storage_index m_storage_index;
    hostports_observer<storage_interface> m_storage_hostports;
    subscriber m_subscriber;
};

} // namespace uh::cluster::storage
