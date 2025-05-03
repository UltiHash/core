#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber/subscriber.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <storage/group/state.h>

namespace uh::cluster::storage {

struct group_initialized {
    static void put(etcd_manager& etcd, std::size_t group_id, bool value) {
        etcd.put_persistant(get_prefix(group_id).group_initialized,
                            serialize(value));
    }

    static auto get(etcd_manager& etcd, std::size_t group_id) {
        return deserialize<bool>(
            etcd.get(get_prefix(group_id).group_initialized));
    };
};

/*
 * Storage-wise publisher
 */
class storage_state_publisher {
public:
    storage_state_publisher(etcd_manager& etcd, std::size_t group_id,
                            std::size_t storage_id)
        : m_etcd{etcd},
          m_prefix{get_prefix(group_id)},
          m_storage_id{storage_id} {}
    ~storage_state_publisher() {
        // NOTE: Do not remove group_initialized, to make it persistant
        m_etcd.rm(m_prefix.storage_states[m_storage_id]);
    }

    void put(storage_state value) {
        m_etcd.put(m_prefix.storage_states[m_storage_id], serialize(value));
    }

    void put_others_persistant(std::size_t id, storage_state value) {
        if (m_storage_id == id) {
            throw std::runtime_error("Cannot put storage state to itself");
        }
        m_etcd.put_persistant(m_prefix.storage_states[id], serialize(value));
    }

private:
    etcd_manager& m_etcd;
    prefix_t m_prefix;
    std::size_t m_storage_id;
};

/*
 * Group-wise subscriber
 */
class storage_state_subscriber {
public:
    using callback_t = vector_observer<storage_state>::callback_t;
    storage_state_subscriber(etcd_manager& etcd, std::size_t group_id,
                             std::size_t num_storages,
                             callback_t callback = nullptr)
        : m_prefix{get_prefix(group_id)},
          m_storage_states{m_prefix.storage_states, num_storages, {}, callback},
          m_subscriber{etcd, m_prefix, {m_storage_states}} {}

    auto get() { return m_storage_states.get(); };

private:
    prefix_t m_prefix;
    vector_observer<storage_state> m_storage_states;
    subscriber m_subscriber;
};

} // namespace uh::cluster::storage
