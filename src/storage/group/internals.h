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
