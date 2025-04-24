#pragma once

#include <common/etcd/endpoints/endpoints.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/service_factory.h>
#include <common/utils/strings.h>
#include <storage/group/state.h>

namespace uh::cluster::storage {

using prefix_t = ns::storage_groups_t::impl_t;

inline prefix_t get_prefix(size_t group_id) {
    return ns::root.storage_groups[group_id];
}

/*
 * Storage-wise publisher
 */
class externals_publisher {
private:
    etcd_manager& m_etcd;
    prefix_t m_prefix;
    size_t m_storage_id;

public:
    externals_publisher(etcd_manager& etcd, size_t group_id, size_t storage_id)
        : m_etcd{etcd},
          m_prefix{get_prefix(group_id)},
          m_storage_id{storage_id} {}
    ~externals_publisher() {
        m_etcd.rm(m_prefix.group_state);
        m_etcd.rm(m_prefix.storage_hostports[m_storage_id]);
        m_etcd.rm(m_prefix.storage_states[m_storage_id]);
    }

    void put_group_state(group_state s) {
        m_etcd.put(m_prefix.group_state, serialize(s));
    }
    void put_storage_hostport(hostport s) {
        m_etcd.put(m_prefix.storage_hostports[m_storage_id], serialize(s));
    }
    void put_storage_state(storage_state s) {
        m_etcd.put(m_prefix.storage_states[m_storage_id], serialize(s));
    }
};

/*
 * Group-wise subscriber
 */
class externals_subscriber {
public:
    externals_subscriber(etcd_manager& etcd, size_t group_id,
                         size_t num_storages,
                         subscriber<prefix_t>::callback_t callback)
        : m_prefix{get_prefix(group_id)},
          m_group_state{m_prefix.group_state},
          m_storage_hostports{m_prefix.storage_hostports, num_storages},
          m_storage_states{m_prefix.storage_states, num_storages},
          m_subscriber{etcd,
                       m_prefix,
                       {m_group_state, m_storage_hostports, m_storage_states},
                       std::move(callback)} {}
    auto get_group_state() { return m_group_state.get(); };
    auto get_storage_hostports() { return m_storage_hostports.get(); };
    auto get_storage_states() { return m_storage_states.get(); };

private:
    prefix_t m_prefix;
    value_observer<group_state> m_group_state;
    vector_observer<hostport> m_storage_hostports;
    vector_observer<storage_state> m_storage_states;
    subscriber<prefix_t> m_subscriber;
};

} // namespace uh::cluster::storage
