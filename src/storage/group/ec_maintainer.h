#pragma once

#include <common/etcd/service.h>
#include <memory>
#include <storage/global/config.h>
#include <storage/group/config.h>
#include <storage/group/externals.h>
#include <storage/group/internals.h>
#include <storage/group/offset.h>

namespace uh::cluster::storage {

template <typename T>
concept SupportsWriteOffset = requires(T t, std::size_t offset) {
    { t.get_write_offset() } -> std::same_as<std::size_t>;
    { t.set_write_offset(offset) } -> std::same_as<void>;
};

template <SupportsWriteOffset T> class ec_maintainer {
public:
    ec_maintainer(boost::asio::io_context& ioc, etcd_manager& etcd,
                  const group_config& group_cfg, std::size_t storage_id,
                  const service_config& service_cfg,
                  const global_data_view_config& gdv_cfg,
                  std::shared_ptr<T> write_offset_interface)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_group_config{group_cfg},
          m_storage_id{storage_id},

          m_write_offset_interface{write_offset_interface},

          m_group_state_manager{etcd, group_cfg.id},

          m_prefix{get_prefix(m_group_config.id)},
          m_group_initialized{m_prefix.group_initialized},
          m_storage_assignment_trigger{
              m_prefix.storage_assignment_trigger,
              [this](bool& val) { assignment_trigger_handler(val); }},
          m_candidate{
              etcd,
              m_prefix.leader,
              (candidate_observer::id_t)storage_id,
              [this](bool is_leader) { election_handler(m_ioc, is_leader); },
              [this](bool is_leader) {
                  (void)is_leader;
                  (void)this;
                  LOG_DEBUG()
                      << "proclaim detected on the group " << m_group_config.id
                      << " storage " << m_storage_id;
              },
          },
          m_storage_states{m_prefix.storage_states, m_group_config.storages},
          m_storage_state_manager{etcd, m_group_config.id, storage_id,
                                  service_cfg.working_dir} {

        m_subscriber.emplace(
            "", etcd, m_prefix,
            std::initializer_list<std::reference_wrapper<subscriber_observer>>{
                std::ref(m_group_initialized),
                std::ref(m_storage_assignment_trigger), std::ref(m_candidate),
                std::ref(m_storage_states)},
            [this]() {
                if (m_candidate.is_leader()) {
                    LOG_DEBUG() << std::format(
                        "[group {}, storage {}] subscriber callback",
                        m_group_config.id, m_storage_id);
                    storage_states_handler();
                }
            });
    }

    ~ec_maintainer() {
        LOG_DEBUG() << std::format("[group {}, storage {}] destroy",
                                   m_group_config.id, m_storage_id);

        m_subscriber.reset();

        std::lock_guard<std::mutex> lock(m_mutex);
        m_thread.reset();
    }

private:
    struct statistics {
        bool has_down = false;
        std::size_t assigned_count = 0ul;
    };

    statistics get_statistics(std::vector<storage_state>& storage_states) {

        statistics rv;
        for (const auto& val : storage_states) {
            switch (val) {
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

    void election_handler(boost::asio::io_context& ioc, bool is_leader) {

        auto offset = m_write_offset_interface->get_write_offset();
        LOG_DEBUG() << std::format("[group {}, storage {}] put offset: {}",
                                   m_group_config.id, m_storage_id, offset);
        offset_manager::put(m_etcd, m_group_config.id, m_storage_id, offset);

        if (is_leader) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_thread.emplace([this]() {
                LOG_DEBUG() << std::format(
                    "[group {}, storage {}] won election, waiting for offsets",
                    m_group_config.id, m_storage_id);

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                auto manager = offset_manager(m_etcd, m_group_config.id,
                                              m_group_config.storages);
                auto offset =
                    manager.summarize_offsets(OFFSET_GATHERING_TIMEOUT);
                LOG_DEBUG() << std::format(
                    "[group {}, storage {}] summarized offset is {}",
                    m_group_config.id, m_storage_id, offset);

                m_write_offset_interface->set_write_offset(offset);

                m_candidate.proclaim();

                LOG_DEBUG()
                    << std::format("[group {}, storage {}] proclaimed election",
                                   m_group_config.id, m_storage_id);
            });

            // std::promise<void> p;
            // {
            //     LOG_DEBUG()
            //         << std::format("[group {}, storage {}] won election",
            //                        m_group_config.id, m_storage_id);
            //
            //     auto manager = offset_manager(m_etcd, m_group_config.id,
            //                                   m_group_config.storages);
            //     auto offset =
            //         manager.summarize_offsets(OFFSET_GATHERING_TIMEOUT);
            //     LOG_DEBUG() << std::format(
            //         "[group {}, storage {}] summarized offset is {}",
            //         m_group_config.id, m_storage_id, offset);
            //
            //     m_write_offset_interface->set_write_offset(offset);
            //
            //     m_candidate.proclaim();
            //
            //     p.set_value();
            // }
        }
    }

    void storage_states_handler() {
        auto group_initialized = m_group_initialized.get();
        auto storage_states = m_storage_states.get();

        auto stats = get_statistics(storage_states);
        auto state = m_group_state_manager.get();

        if (not(group_initialized)) {
            LOG_DEBUG() << std::format(
                "[group {}, storage {}] group uninitialized: has_down: {}, "
                "assigned_count: {}",
                m_group_config.id, m_storage_id, stats.has_down,
                stats.assigned_count);
            // LOG_DEBUG() << "group initialized: " +
            // serialize(group_initialized_manager::get(m_etcd,
            // m_group_config.id));
            if (stats.has_down)
                return;

            auto trigger = m_storage_assignment_trigger.get();
            if (not(trigger)) {

                LOG_DEBUG()
                    << std::format("[group {}, storage {}] trigger "
                                   "assigning storages: This "
                                   "should be done only once",
                                   m_group_config.id, m_storage_id,
                                   stats.assigned_count, stats.has_down);

                storage_assignment_triggers_manager::put(
                    m_etcd, m_group_config.id, true);
            }

            if (stats.assigned_count != m_group_config.storages)
                return;

            storage_assignment_triggers_manager::put(m_etcd, m_group_config.id,
                                                     false);
            m_storage_assignment_trigger.set(false);

            LOG_DEBUG() << std::format(
                "[group {}, storage {}] Group is now initialized",
                m_group_config.id, m_storage_id);
            group_initialized_manager::put_persistant(m_etcd, m_group_config.id,
                                                      true);
            m_group_initialized.set(true);
        }

        auto new_state = state;

        if (state != group_state::HEALTHY and
            stats.assigned_count == m_group_config.storages) {
            new_state = group_state::HEALTHY;
        }

        if (state != group_state::DEGRADED and //
            stats.has_down and
            stats.assigned_count >= m_group_config.data_shards) {
            new_state = group_state::DEGRADED;
        }

        if (state != group_state::FAILED and //
            stats.assigned_count < m_group_config.data_shards) {
            new_state = group_state::FAILED;
        }

        if (state != group_state::REPAIRING and //
            !stats.has_down and
            stats.assigned_count >= m_group_config.data_shards and
            stats.assigned_count < m_group_config.storages) {
            new_state = group_state::REPAIRING;

            // TODO: Spawn repairer here
        }

        if (new_state != state) {
            m_group_state_manager.put(new_state);

            LOG_DEBUG() << std::format(
                "[group {}, storage {}] assigned_count {}, has_down {}",
                m_group_config.id, m_storage_id, stats.assigned_count,
                stats.has_down);
            LOG_DEBUG() << std::format(
                "[group {}, storage {}] change group state to {}",
                m_group_config.id, m_storage_id,
                magic_enum::enum_name(new_state));
        }
    }

    void assignment_trigger_handler(bool& val) {
        if (m_storage_state_manager.get() != storage_state::ASSIGNED and val) {

            LOG_DEBUG() << std::format(
                "[group {}, storage {}] set it's state to ASSIGNED",
                m_group_config.id, m_storage_id);

            m_storage_state_manager.put(storage_state::ASSIGNED);
        }
    }

    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    const group_config& m_group_config;
    std::size_t m_storage_id;

    std::shared_ptr<T> m_write_offset_interface;

    group_state_manager m_group_state_manager;

    prefix_t m_prefix;

    std::mutex m_mutex;
    std::optional<std::jthread> m_thread;

    /*
     * subscriber's observers
     */
    sync_value_observer<bool> m_group_initialized;
    sync_value_observer<bool> m_storage_assignment_trigger;
    candidate_observer m_candidate; // It removes leader key on it's destructor
    sync_vector_observer<storage_state> m_storage_states;

    // NOTE: Order is important! The storage state should be destroyed
    // before the leader key, which is handled by the candidate_observer.
    storage_state_manager m_storage_state_manager; // It removes storage state
                                                   // on it's destructor
    std::optional<subscriber> m_subscriber;
};

} // namespace uh::cluster::storage
