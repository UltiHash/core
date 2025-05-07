#pragma once

#include "impl/prefix.h"

#include <common/etcd/subscriber.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <storage/group/state.h>

namespace uh::cluster::storage {

struct etcd_group_initialized {
    static void put_persistant(etcd_manager& etcd, std::size_t group_id,
                               bool value) {
        etcd.put_persistant(get_prefix(group_id).group_initialized,
                            serialize(value));
    }

    static auto get(etcd_manager& etcd, std::size_t group_id) {
        return deserialize<bool>(
            etcd.get(get_prefix(group_id).group_initialized));
    };
};

struct etcd_storage_assignment_triggers {
    static void put(etcd_manager& etcd, std::size_t group_id,
                    std::size_t storage_id, bool value) {
        etcd.put(get_prefix(group_id).storage_assignment_triggers[storage_id],
                 serialize(value));
    }

    static auto get(etcd_manager& etcd, std::size_t group_id,
                    std::size_t storage_id) {
        return deserialize<bool>(etcd.get(
            get_prefix(group_id).storage_assignment_triggers[storage_id]));
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
          m_group_initialized{m_prefix.group_initialized},
          m_storage_states{
              m_prefix.storage_states, num_storages,
              [=](std::size_t id, storage_state& state) {
                  LOG_DEBUG() << std::format(
                      "[group {}, storage {}] Storage {} state changed to {}",
                      group_id, storage_id, id, magic_enum::enum_name(state));
              }},
          m_storage_assignment_triggers{m_prefix.storage_assignment_triggers,
                                        num_storages},
          m_candidate{etcd, m_prefix.leader,
                      (candidate_observer::id_t)storage_id,
                      std::move(election_callback)},
          m_subscriber{"internals_subscriber",
                       etcd,
                       m_prefix,
                       {m_group_initialized, m_storage_states,
                        m_storage_assignment_triggers, m_candidate},
                       std::move(callback)} {}

    value_observer<bool>& group_initialized() { return m_group_initialized; }
    vector_observer<storage_state>& storage_states() {
        return m_storage_states;
    }
    vector_observer<bool>& storage_assignment_triggers() {
        return m_storage_assignment_triggers;
    }
    candidate_observer& candidate() { return m_candidate; }

private:
    prefix_t m_prefix;
    value_observer<bool> m_group_initialized;
    vector_observer<storage_state> m_storage_states;
    vector_observer<bool> m_storage_assignment_triggers;
    candidate_observer m_candidate;
    subscriber m_subscriber;
};

} // namespace uh::cluster::storage
