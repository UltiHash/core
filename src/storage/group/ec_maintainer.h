#pragma once

#include <common/etcd/registry/storage_registry.h>
#include <common/etcd/service.h>
#include <memory>
#include <storage/group/state_manager.h>

namespace uh::cluster::storage {

struct participant {
    ~participant() = default;
};

class leader : public participant {
public:
    leader(boost::asio::io_context& ioc, etcd_manager& etcd,
           const group_config& group_config, std::size_t storage_id,
           const global_data_view_config& gdv_config,
           storage_registry& storage_registry) {

        if (group_initialized::get(etcd, group_config.id) == false) {
            auto storage_states =
                wait_until_no_down_storage(etcd, group_config, storage_id);

            for (auto i = 0ul; i < storage_states.size(); ++i) {
                if (storage_states[i] == storage_state::NEW) {
                    if (i == storage_id) {
                        storage_registry.update_service_state(
                            storage_state::ASSIGNED);
                    } else {
                        auto publisher = storage_state_publisher(
                            etcd, group_config.id, storage_id);
                        publisher.put_others_persistant(
                            i, storage_state::ASSIGNED);
                    }
                }
            }

            group_initialized::put(etcd, group_config.id, true);

        } else {
            // To wait for the offset values from other storage services.
            std::this_thread::sleep_for(std::chrono::milliseconds(700));
        }

        m_offset = leader::summarize_offsets(etcd, group_config.id, storage_id);

        m_state_manager.emplace(ioc, etcd, group_config, storage_id,
                                gdv_config);
    }

private:
    static std::size_t summarize_offsets(etcd_manager& etcd,
                                         std::size_t group_id,
                                         std::size_t num_storages) {
        auto os = offset_subscriber(etcd, group_id, num_storages);
        auto offsets = os.get();

        auto max_offset = std::ranges::max_element(
            offsets,
            []<typename T>(const T& a, const T& b) { return *a < *b; });

        return max_offset != offsets.end() ? **max_offset : 0;
    }

    static std::vector<storage_state>
    wait_until_no_down_storage(etcd_manager& etcd,
                               const group_config& group_config,
                               std::size_t storage_id) {
        auto promise = std::promise<void>();
        auto future = promise.get_future();
        storage_state_subscriber subscriber{
            etcd, group_config.id, group_config.storages,
            [&subscriber, &promise](std::size_t id, storage_state state) {
                auto storage_states = subscriber.get();
                auto no_down_storage =
                    std::ranges::all_of(storage_states, [](const auto& state) {
                        return *state != storage_state::DOWN;
                    });
                if (no_down_storage)
                    promise.set_value();
            }};
        future.wait(); // NOTE: there's no timeout for now.

        auto storage_states = subscriber.get();
        std::vector<storage_state> rv;
        rv.reserve(storage_states.size());
        std::ranges::transform(storage_states, std::back_inserter(rv),
                               [](const auto& state) { return *state; });
        return rv;
    }

    std::size_t m_offset;
    std::optional<state_manager> m_state_manager;
};

class follower : public participant {
public:
    follower(etcd_manager& etcd, const group_config& group_config,
             std::size_t storage_id, storage_registry& storage_registry)
        : m_subscriber{etcd, group_config.id, group_config.storages,
                       [&](std::size_t id, storage_state state) {
                           if (id == storage_id) {
                               storage_registry.update_service_state(state);
                           }
                       }} {}

private:
    storage_state_subscriber m_subscriber;
};

class ec_maintainer {
public:
    ec_maintainer(boost::asio::io_context& ioc, etcd_manager& etcd,
                  const group_config& group_config, std::size_t storage_id,
                  const service_config& service_config,
                  const global_data_view_config& gdv_config)
        : m_offset_publisher{etcd, group_config.id, storage_id},
          m_storage_registry{etcd, group_config.id, storage_id,
                             service_config.working_dir},
          m_candidate(
              etcd, get_prefix(group_config.id).leader, serialize(storage_id),
              [&](bool is_leader) {
                  // TODO: Get offset from local storage
                  std::size_t current_offset = 0;
                  m_offset_publisher.put(current_offset);

                  if (is_leader) {
                      LOG_INFO() << std::format("Storage service {} is elected "
                                                "as a leader of group {}",
                                                storage_id, group_config.id);
                      m_participant = std::make_unique<leader>(
                          ioc, etcd, group_config, storage_id, gdv_config,
                          m_storage_registry);
                  } else /*follower */ {
                      LOG_INFO() << std::format("Storage service {} is a "
                                                "follower of group {}",
                                                storage_id, group_config.id);

                      m_participant = std::make_unique<follower>(
                          etcd, group_config, storage_id, m_storage_registry);
                  }
              }) {}

private:
    offset_publisher m_offset_publisher;
    storage_registry m_storage_registry;
    std::unique_ptr<participant> m_participant;
    candidate m_candidate;
};

} // namespace uh::cluster::storage
