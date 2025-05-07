#pragma once

#include <common/etcd/registry/storage_registry.h>
#include <common/etcd/service.h>
#include <memory>
#include <storage/group/state_manager.h>

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
          m_group_state_manager(ioc, etcd, group_cfg, storage_id, gdv_cfg,
                                m_storage_registry),
          m_subscriber{m_etcd,
                       m_group_config.id,
                       m_group_config.storages,
                       storage_id,
                       [this](bool is_leader) { election_callback(is_leader); },
                       [this]() { handler(); }} {}

    void election_callback(bool is_leader) {
        // TODO: Get offset from local storage
        std::size_t current_offset = 0;
        m_offset_publisher.put(current_offset);

        if (is_leader) {
            LOG_DEBUG() << std::format("[group {}] storage {} is the leader",
                                       m_group_config.id, m_storage_id);
            m_offset = summarize_offsets();

            m_subscriber.candidate().proclaim();
        } else {
            LOG_DEBUG() << std::format("[group {}] storage {} is a follower",
                                       m_group_config.id, m_storage_id);
        }
    }

private:
    void handler() {
        auto is_leader = m_subscriber.candidate().is_leader();

        if (is_leader) {
            m_group_state_manager.manage_state(
                group_initialized::get(m_etcd, m_group_config.id),
                m_subscriber.storage_states().get());
        } else { // follower
            auto state = m_subscriber.storage_states().get(m_storage_id);
            if (*state == storage_state::ASSIGNED) {
                m_storage_registry.set(storage_state::ASSIGNED);
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
