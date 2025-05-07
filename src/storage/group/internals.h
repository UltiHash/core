#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber.h>

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
class internals_subscriber {
public:
    // TODO: Use callback of subscriber, rather than using vector_observer's.
    using election_callback_t = candidate_observer::callback_t;
    using callback_t = subscriber::callback_t;
    internals_subscriber(etcd_manager& etcd, std::size_t group_id,
                         std::size_t num_storages, std::size_t storage_id,
                         election_callback_t election_callback = nullptr,
                         callback_t callback = nullptr)
        : m_prefix{get_prefix(group_id)},
          m_storage_states{m_prefix.storage_states, num_storages, {}},
          m_candidate{etcd, m_prefix.leader,
                      (candidate_observer::id_t)storage_id,
                      std::move(election_callback)},
          m_subscriber{"internals_subscriber",
                       etcd,
                       m_prefix,
                       {m_storage_states, m_candidate},
                       std::move(callback)} {}

    const candidate_observer& candidate() const { return m_candidate; }
    const vector_observer<storage_state>& storage_states() const {
        return m_storage_states;
    }

private:
    prefix_t m_prefix;
    vector_observer<storage_state> m_storage_states;
    candidate_observer m_candidate;
    subscriber m_subscriber;
};

} // namespace uh::cluster::storage
