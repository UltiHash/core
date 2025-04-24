#pragma once

#include "externals.h"
#include "internals.h"
#include "offset.h"

#include <algorithm>
#include <common/etcd/candidate.h>
#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <functional>
#include <optional>

namespace uh::cluster::storage {

struct storage_campaign_strategy : public campaign_strategy {
    storage_campaign_strategy(etcd_manager& etcd, size_t group_id,
                              size_t num_storages, size_t storage_id,
                              std::function<void()> manage_state)
        : m_etcd{etcd},
          m_group_id{group_id},
          m_num_storages{num_storages},
          m_storage_id{storage_id},
          m_manage_state{std::move(manage_state)} {}

    void pre_campaign() override {
        m_offset_publisher.emplace(m_etcd, m_group_id, m_storage_id);

        // TODO: get offset
        size_t current_offset = 0;
        m_offset_publisher->put(current_offset);
    }

    void on_elected() override {
        auto offsets =
            offset_subscriber(m_etcd, m_group_id, m_num_storages).get();

        auto max_offset = std::ranges::max_element(
            offsets,
            []<typename T>(const T& a, const T& b) { return *a < *b; });

        // TODO: set offset using max_offset
        (void)max_offset;

        m_manage_state();
    }

    void post_campaign() override { m_offset_publisher.reset(); }

private:
    etcd_manager& m_etcd;
    size_t m_group_id;
    size_t m_num_storages;
    size_t m_storage_id;
    std::function<void()> m_manage_state;
    std::optional<offset_publisher> m_offset_publisher;
};

/*
 * This class manages group state only when it is the leader of the group.
 */
class state_manager {
public:
    state_manager(etcd_manager& etcd, size_t group_id, size_t num_storages,
                  size_t storage_id)
        : m_etcd{etcd},
          m_group_id{group_id},
          m_num_storages{num_storages},
          m_storage_id{storage_id},
          m_externals_publisher(etcd, group_id, storage_id),
          m_candidate(m_etcd, get_prefix(m_group_id).leader,
                      serialize(m_storage_id),
                      std::make_unique<storage_campaign_strategy>(
                          etcd, group_id, num_storages, storage_id,
                          // State management function
                          [this]() {
                              m_group_state.emplace(group_state::INITIALIZING);
                              internals_subscriber.emplace(
                                  m_etcd, m_group_id, m_num_storages,
                                  [this](etcd_manager::response) { manage(); });
                          })) {}

private:
    /*
     * Called only on a leader
     */
    void manage() {
        auto group_initialized = internals_subscriber->get_group_initialized();
        auto storage_states = internals_subscriber->get_storage_states();

        // TODO: manage state here
        // m_group_state = group_state::INITIALIZING;
        // m_externals_publisher.put_group_state(*m_group_state);
    }

    etcd_manager& m_etcd;
    size_t m_group_id;
    size_t m_num_storages;
    size_t m_storage_id;
    externals_publisher m_externals_publisher;

    /*
     * Used only on a leader
     */
    std::optional<group_state> m_group_state;
    std::optional<internals_subscriber> internals_subscriber;

    candidate m_candidate;
};

} // namespace uh::cluster::storage
