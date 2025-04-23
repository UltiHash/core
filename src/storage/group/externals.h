#pragma once

#include <storage/group/state.h>

#include <common/telemetry/log.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/service_interfaces/service_factory.h>
#include <common/utils/strings.h>
#include <filesystem>

namespace uh::cluster::storage::externals {

using prefix_t = ns::storage_groups_t::impl_t;

inline prefix_t get_prefix(size_t group_id) {
    return ns::root.storage_groups[group_id];
}

/*
 * Storage-wise publisher
 */
class publisher {
public:
    publisher(etcd_manager& etcd, size_t group_id, size_t storage_id)
        : m_etcd{etcd},
          m_prefix{get_prefix(group_id)},
          m_storage_id{storage_id} {}
    ~publisher() {
        m_etcd.rm(m_prefix.group_state);
        m_etcd.rm(m_prefix.storage_hostports[m_storage_id]);
        m_etcd.rm(m_prefix.storage_states[m_storage_id]);
    }
    void put_group_state(group_state s) {
        m_etcd.put(m_prefix.group_state, serialize(s));
    }

    void put_storage_hostport(hostport storage_hostport) {
        m_etcd.put(m_prefix.storage_hostports[m_storage_id],
                   serialize(storage_hostport));
    }

    void put_storage_state(storage_state s) {
        m_etcd.put(m_prefix.storage_states[m_storage_id], serialize(s));
    }

private:
    etcd_manager& m_etcd;
    prefix_t m_prefix;
    size_t m_storage_id;
};

/*
 * Group-wise subscriber
 */
class subscriber {
public:
    using group_state_callback_t = std::function<void(group_state*)>;
    using storage_hostport_callback_t = std::function<void(hostport*)>;
    using storage_states_callback_t = std::function<void(storage_state*)>;
    subscriber(etcd_manager& etcd, size_t group_id, size_t num_storages,
               group_state_callback_t group_state_callback = nullptr,
               storage_hostport_callback_t storage_hostport_callback = nullptr,
               storage_states_callback_t storage_states_callback = nullptr)
        : m_etcd{etcd},
          m_group_state_callback{std::move(group_state_callback)},
          m_storage_hostport_callback{std::move(storage_hostport_callback)},
          m_storage_states_callback{std::move(storage_states_callback)},
          m_prefix{get_prefix(group_id)},
          m_storage_hostports(num_storages),
          m_storage_states(num_storages) {

        m_group_state.store(std::make_shared<group_state>(),
                            std::memory_order_release);
        for (auto& atom : m_storage_hostports) {
            atom.store(std::make_shared<hostport>(), std::memory_order_release);
        }
        for (auto& atom : m_storage_states) {
            atom.store(std::make_shared<storage_state>(),
                       std::memory_order_release);
        }
        m_wg = m_etcd.watch(
            m_prefix, [this](etcd_manager::response resp) { on_watch(resp); });
    }

    std::shared_ptr<group_state> get_group_state() const {
        return m_group_state.load(std::memory_order_acquire);
    }

    auto get_hostport(size_t storage_id) const {
        return m_storage_hostports[storage_id].load(std::memory_order_acquire);
    }

    std::vector<std::shared_ptr<hostport>> get_hostports() const {
        std::vector<std::shared_ptr<hostport>> result;
        result.reserve(m_storage_hostports.size());
        for (const auto& atom : m_storage_hostports) {
            result.push_back(atom.load(std::memory_order_acquire));
        }
        return result;
    }

    auto get_storage_state(size_t storage_id) const {
        return m_storage_states[storage_id].load(std::memory_order_acquire);
    }

    std::vector<std::shared_ptr<storage_state>> get_storage_states() const {
        std::vector<std::shared_ptr<storage_state>> result;
        result.reserve(m_storage_states.size());
        for (const auto& atom : m_storage_states) {
            result.push_back(atom.load(std::memory_order_acquire));
        }
        return result;
    }

private:
    void on_watch(etcd_manager::response resp) {
        try {
            LOG_INFO() << "externals_watcher has detected a change on "
                       << resp.key;

            if (resp.key == std::string(m_prefix.group_state)) {
                switch (get_etcd_action_enum(resp.action)) {
                case etcd_action::GET:
                case etcd_action::CREATE:
                case etcd_action::SET: {
                    auto s = deserialize<group_state>(resp.value);
                    m_group_state.store(std::make_shared<group_state>(s));

                    LOG_INFO()
                        << "Modified group_state " << resp.value << " saved";

                    if (m_group_state_callback)
                        m_group_state_callback(get_group_state().get());

                    break;
                }
                case etcd_action::DELETE:
                default:
                    m_group_state.store(std::make_shared<group_state>());

                    LOG_INFO() << "group_state deleted";

                    if (m_group_state_callback)
                        m_group_state_callback(get_group_state().get());
                    break;
                }

            } else if (auto key = std::filesystem::path(resp.key);
                       key.parent_path() ==
                       std::string(m_prefix.storage_hostports)) {
                LOG_DEBUG() << "Storage index " << key.filename().string();
                auto storage_id = stoul(key.filename().string());
                switch (get_etcd_action_enum(resp.action)) {
                case etcd_action::GET:
                case etcd_action::CREATE:
                case etcd_action::SET: {
                    auto t = deserialize<hostport>(resp.value);
                    m_storage_hostports[storage_id].store(
                        std::make_shared<hostport>(t));

                    LOG_INFO() << "Modified hostport for storage " << resp.value
                               << " saved";

                    if (m_storage_hostport_callback)
                        m_storage_hostport_callback(
                            get_hostport(storage_id).get());
                    break;
                }
                case etcd_action::DELETE:
                default:
                    m_storage_hostports[storage_id].store(
                        std::make_shared<hostport>());

                    LOG_INFO() << "Hostport for storage deleted";

                    if (m_storage_hostport_callback)
                        m_storage_hostport_callback(
                            get_hostport(storage_id).get());
                    break;
                }
            } else if (auto key = std::filesystem::path(resp.key);
                       key.parent_path() ==
                       std::string(m_prefix.storage_states)) {
                LOG_DEBUG() << "Storage index " << key.filename().string();
                auto storage_id = stoul(key.filename().string());
                switch (get_etcd_action_enum(resp.action)) {
                case etcd_action::GET:
                case etcd_action::CREATE:
                case etcd_action::SET: {
                    auto state = deserialize<storage_state>(resp.value);

                    m_storage_states[storage_id].store(
                        std::make_shared<storage_state>(state));

                    LOG_INFO() << "Modified state for storage " << resp.value
                               << " saved";

                    if (m_storage_states_callback)
                        m_storage_states_callback(
                            get_storage_state(storage_id).get());
                    break;
                }
                case etcd_action::DELETE:
                default:
                    m_storage_states[storage_id].store(
                        std::make_shared<storage_state>(storage_state::DOWN));

                    LOG_INFO()
                        << "State for storage " << resp.value << " deleted";

                    if (m_storage_states_callback)
                        m_storage_states_callback(
                            get_storage_state(storage_id).get());
                    break;
                }
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error updating externals: " << e.what();
        }
    }

    etcd_manager& m_etcd;
    group_state_callback_t m_group_state_callback;
    storage_hostport_callback_t m_storage_hostport_callback;
    storage_states_callback_t m_storage_states_callback;
    prefix_t m_prefix;
    std::atomic<std::shared_ptr<group_state>> m_group_state;
    std::vector<std::atomic<std::shared_ptr<hostport>>> m_storage_hostports;
    std::vector<std::atomic<std::shared_ptr<storage_state>>> m_storage_states;
    etcd_manager::watch_guard m_wg;
};

} // namespace uh::cluster::storage::externals
