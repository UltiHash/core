#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber/subscriber.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/service_factory.h>
#include <common/utils/strings.h>
#include <storage/group/state.h>

namespace uh::cluster::storage {

/*
 * Storage-wise publisher
 */
class internals_publisher {
public:
    internals_publisher(etcd_manager& etcd, std::size_t group_id,
                        std::size_t storage_id)
        : m_etcd{etcd},
          m_prefix{get_prefix(group_id)},
          m_storage_id{storage_id} {}
    ~internals_publisher() {
        // NOTE: Do not remove group_initialized, to make it persistant
        m_etcd.rm(m_prefix.storage_states[m_storage_id]);
    }

    void put_storage_state(storage_state value) {
        m_etcd.put(m_prefix.storage_states[m_storage_id], serialize(value));
    }

    static void set_group_initialized(etcd_manager& etcd,
                                      std::size_t group_id) {
        etcd.put_persistant(get_prefix(group_id).group_initialized,
                            serialize(true));
    }

private:
    etcd_manager& m_etcd;
    prefix_t m_prefix;
    std::size_t m_storage_id;
};

/*
 * Group-wise subscriber
 */
class internals_subscriber {
public:
    using callback_t = subscriber<prefix_t>::callback_t;
    internals_subscriber(etcd_manager& etcd, std::size_t group_id,
                         std::size_t num_storages,
                         callback_t callback = nullptr)
        : m_prefix{get_prefix(group_id)},
          m_group_initialized{m_prefix.group_initialized},
          m_storage_states{m_prefix.storage_states, num_storages},
          m_subscriber{etcd,
                       m_prefix,
                       {m_group_initialized, m_storage_states},
                       std::move(callback)} {}
    auto get_group_initialized() { return m_group_initialized.get(); };
    auto get_storage_states() { return m_storage_states.get(); };

private:
    prefix_t m_prefix;
    value_observer<bool> m_group_initialized;
    vector_observer<storage_state> m_storage_states;
    subscriber<prefix_t> m_subscriber;
};

} // namespace uh::cluster::storage
