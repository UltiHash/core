#pragma once

#include <common/etcd/registry/storage_registry.h>
#include <common/etcd/service.h>
#include <memory>
#include <storage/global/config.h>
#include <storage/group/config.h>
#include <storage/group/externals.h>
#include <storage/group/internals.h>
#include <storage/group/offset.h>

namespace uh::cluster::storage {

class ec_maintainer : public std::enable_shared_from_this<ec_maintainer> {
public:
    ec_maintainer(boost::asio::io_context& ioc, etcd_manager& etcd,
                  const group_config& group_cfg, std::size_t storage_id,
                  const service_config& service_cfg,
                  const global_data_view_config& gdv_cfg)
        : m_etcd{etcd},
          m_group_config{group_cfg},
          m_storage_id{storage_id},
          m_offset_publisher{etcd, group_cfg.id, storage_id},
          m_storage_registry{etcd, group_cfg.id, storage_id,
                             service_cfg.working_dir},

          m_group_state_manager{etcd, group_cfg.id},

          m_subscriber{m_etcd,
                       m_group_config.id,
                       m_group_config.storages,
                       storage_id,
                       [this](bool is_leader) { election_callback(is_leader); },
                       [this]() { handler(); }} {}

    ~ec_maintainer() {
        LOG_DEBUG() << std::format(
            "Group {}'s maintainer (storage {}) destruction started",
            m_group_config.id, m_storage_id);
    }

    void election_callback(bool is_leader) {
        // TODO: Get offset from local storage
        std::size_t current_offset = 0;
        m_offset_publisher.put(current_offset);

        if (is_leader) {
            LOG_DEBUG() << std::format("[group {}, storage {}] won election",
                                       m_group_config.id, m_storage_id);
            m_offset = summarize_offsets();

            m_subscriber.candidate().proclaim();

        } else {
            LOG_DEBUG() << std::format("[group {}, storage {}] loose election",
                                       m_group_config.id, m_storage_id);
        }
    }

    void manage_state() {
        auto group_initialized = m_subscriber.group_initialized().get();
        auto storage_states = m_subscriber.storage_states().get();

        auto stats = get_statistics(storage_states);

        LOG_DEBUG() << std::format(
            "[group {}, storage {}] assigned_count {}, has_down {}",
            m_group_config.id, m_storage_id, stats.assigned_count,
            stats.has_down);

        if (not(*group_initialized)) {
            LOG_DEBUG() << std::format(
                "[group {}, storage {}] Group isn't initialized",
                m_group_config.id, m_storage_id);

            if (stats.has_down)
                return;

            for (auto i = 0ul; i < storage_states.size(); ++i) {
                if (*storage_states[i] == storage_state::NEW) {
                    if (i == m_storage_id) {
                        LOG_DEBUG()
                            << std::format("[group {}, storage {}] Leader set "
                                           "it's state as ASSIGNED",
                                           m_group_config.id, m_storage_id);

                        m_storage_registry.set(storage_state::ASSIGNED);
                    } else {
                        LOG_DEBUG()
                            << std::format("[group {}, storage {}] Trigger "
                                           "storage {} to be assigned",
                                           m_group_config.id, m_storage_id, i);
                        etcd_storage_assignment_triggers::put(
                            m_etcd, m_group_config.id, i, true);
                        m_subscriber.storage_assignment_triggers().set(i, true);
                    }
                    m_subscriber.storage_states().set(i,
                                                      storage_state::ASSIGNED);
                }
            }
            if (stats.assigned_count != m_group_config.storages)
                return;

            LOG_DEBUG() << std::format(
                "[group {}, storage {}] Group is now initialized",
                m_group_config.id, m_storage_id);
            etcd_group_initialized::put_persistant(m_etcd, m_group_config.id,
                                                   true);
            m_subscriber.group_initialized().set(true);
        }

        auto group_state = m_group_state_manager.get();
        if (group_state != group_state::HEALTHY and
            stats.assigned_count == m_group_config.storages) {
            m_group_state_manager.set(group_state::HEALTHY);
        }

        if (group_state != group_state::DEGRADED and //
            stats.has_down and
            stats.assigned_count >= m_group_config.data_shards) {
            m_group_state_manager.set(group_state::DEGRADED);
        }

        if (group_state != group_state::FAILED and //
            stats.assigned_count < m_group_config.data_shards) {
            m_group_state_manager.set(group_state::FAILED);
        }

        if (group_state != group_state::REPAIRING and //
            !stats.has_down and
            stats.assigned_count >= m_group_config.data_shards and
            stats.assigned_count < m_group_config.storages) {
            m_group_state_manager.set(group_state::REPAIRING);

            // TODO: Spawn repairer here
        }
    }

private:
    struct statistics {
        bool has_down = false;
        std::size_t assigned_count = 0ul;
    };

    statistics get_statistics(
        std::vector<std::shared_ptr<storage_state>>& storage_states) {

        statistics rv;
        for (auto cnt = 0ul; const auto& val : storage_states) {
            LOG_DEBUG() << std::format(
                "[group {}, storage {}] storage {} state: {}",
                m_group_config.id, m_storage_id, cnt++,
                magic_enum::enum_name(*val));
            switch (*val) {
            case storage_state::DOWN:
                rv.has_down = true;
                break;
            case storage_state::ASSIGNED:
                rv.assigned_count++;
                break;
            default:
                break;
            }
        }
        return rv;
    }

    void handler() {
        auto is_leader = m_subscriber.candidate().is_leader();

        if (is_leader) {
            LOG_DEBUG() << std::format(
                "[group {}, storage {}] @@@@@@@@@@@@@@@@@@@@@@ I'm a leader",
                m_group_config.id, m_storage_id);
            manage_state();

        } else { // follower
            LOG_DEBUG() << std::format(
                "[group {}, storage {}] @@@@@@@@@@@@@@@@@@@@@@ I'm a follower",
                m_group_config.id, m_storage_id);

            auto trigger =
                m_subscriber.storage_assignment_triggers().get(m_storage_id);
            if (*trigger) {
                LOG_DEBUG() << std::format(
                    "[group {}, storage {}] Got assignment trigger",
                    m_group_config.id, m_storage_id);
                m_storage_registry.set(storage_state::ASSIGNED);
                // m_subscriber.storage_states().set(m_storage_id,
                //                                   storage_state::ASSIGNED);
                etcd_storage_assignment_triggers::put(m_etcd, m_group_config.id,
                                                      m_storage_id, false);
                m_subscriber.storage_assignment_triggers().set(m_storage_id,
                                                               false);
            }
        }
    }

    std::size_t summarize_offsets() {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        // TODO: use reader instead of subscriber
        auto reader =
            offset_reader(m_etcd, m_group_config.id, m_group_config.storages);
        auto offsets = reader.get();

        auto max_offset = std::ranges::max_element(
            offsets,
            []<typename T>(const T& a, const T& b) { return *a < *b; });

        return max_offset != offsets.end() ? **max_offset : 0;
    }

    etcd_manager& m_etcd;
    const group_config& m_group_config;
    std::size_t m_storage_id;

    std::atomic<std::size_t> m_offset;
    offset_publisher m_offset_publisher;

    storage_registry m_storage_registry;

    group_state_manager m_group_state_manager;

    internals_subscriber m_subscriber;
};

} // namespace uh::cluster::storage
