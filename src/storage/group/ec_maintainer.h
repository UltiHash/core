#pragma once

#include <common/etcd/registry/storage_registry.h>
#include <common/etcd/service.h>
#include <memory>
#include <storage/group/state_manager.h>

namespace uh::cluster::storage {

struct participant {
    virtual ~participant() = default;
};

class leader : public participant {
public:
    leader(boost::asio::io_context& ioc, etcd_manager& etcd,
           const group_config& group_cfg, std::size_t storage_id,
           const global_data_view_config& gdv_cfg,
           storage_registry& storage_registry)
        : m_etcd{etcd},
          m_group_config{group_cfg},
          m_storage_id{storage_id} {
        LOG_DEBUG() << std::format(
            "Group {}'s leader (storage {}) construction is started",
            m_group_config.id, m_storage_id);
        m_offset = summarize_offsets();
        m_group_state_manager.emplace(ioc, etcd, group_cfg, storage_id, gdv_cfg,
                                      storage_registry);
        LOG_DEBUG() << std::format(
            "Group {}'s leader (storage {}) construction is done",
            m_group_config.id, m_storage_id);
    }

    ~leader() {
        LOG_DEBUG() << std::format(
            "Group {}'s leader (storage {}) destruction started",
            m_group_config.id, m_storage_id);
    }

private:
    std::size_t summarize_offsets() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
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

    std::size_t m_offset;
    std::optional<group_state_manager> m_group_state_manager;
};

class follower : public participant {
public:
    follower(etcd_manager& etcd, const group_config& group_cfg,
             std::size_t storage_id, storage_registry& storage_registry)
        : m_group_config{group_cfg},
          m_storage_id{storage_id},
          m_key{
              ns::root.storage_groups[group_cfg.id].storage_states[storage_id]},
          m_storage_state{m_key,
                          {},
                          [&](storage_state& state) {
                              if (state == storage_state::ASSIGNED)
                                  storage_registry.set(state);
                          }},
          m_subscriber{"follower", etcd, m_key, {m_storage_state}} {
        LOG_DEBUG() << std::format(
            "Group {}'s follower (storage {}) is constructed",
            m_group_config.id, m_storage_id);
    }

    ~follower() {
        LOG_DEBUG() << std::format(
            "Group {}'s follower (storage {}) distruction started",
            m_group_config.id, m_storage_id);
    }

private:
    const group_config& m_group_config;
    std::size_t m_storage_id;

    std::string m_key;

    value_observer<storage_state> m_storage_state;
    subscriber m_subscriber;
};

// TODO: write a test for ec_maintainer

class ec_maintainer : public std::enable_shared_from_this<ec_maintainer> {
public:
    ec_maintainer(boost::asio::io_context& ioc, etcd_manager& etcd,
                  const group_config& group_cfg, std::size_t storage_id,
                  const service_config& service_cfg,
                  const global_data_view_config& gdv_cfg)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_group_config{group_cfg},
          m_storage_id{storage_id},
          m_service_config{service_cfg},
          m_gdv_config{gdv_cfg},
          m_offset_publisher{etcd, group_cfg.id, storage_id},
          m_storage_registry{etcd, group_cfg.id, storage_id,
                             m_service_config.working_dir},
          m_candidate(
              m_etcd, get_prefix(m_group_config.id).leader, m_storage_id,
              [this](bool is_leader) { election_callback(is_leader); }) {

        LOG_DEBUG() << std::format(
            "Group {}'s maintainer (storage {}) is constructed",
            m_group_config.id, m_storage_id);
    }

    void election_callback(bool is_leader) {
        // TODO: Get offset from local storage
        std::size_t current_offset = 0;
        m_offset_publisher.put(current_offset);
        if (m_participant) {
            m_participant.reset();
            LOG_DEBUG() << std::format("Group {}'s leader/follower (storage "
                                       "{}) distruction done",
                                       m_group_config.id, m_storage_id);
        }

        if (is_leader) {
            m_participant = std::make_unique<leader>(
                m_ioc, m_etcd, m_group_config, m_storage_id, m_gdv_config,
                m_storage_registry);

            LOG_INFO() << "Leader creation is done";

        } else /*follower */ {
            m_participant = std::make_unique<follower>(
                m_etcd, m_group_config, m_storage_id, m_storage_registry);
        }
    }

    ~ec_maintainer() {
        LOG_DEBUG() << std::format(
            "Group {}'s maintainer (storage {}) destruction is started",
            m_group_config.id, m_storage_id);
    }

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    const group_config& m_group_config;
    std::size_t m_storage_id;
    const service_config& m_service_config;
    const global_data_view_config& m_gdv_config;

    offset_publisher m_offset_publisher;
    storage_registry m_storage_registry;
    std::unique_ptr<participant> m_participant;
    candidate m_candidate;
};

} // namespace uh::cluster::storage
