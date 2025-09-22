#pragma once

#include "subscriber_observer.h"

#include <common/etcd/service_discovery/service_observer.h>
#include <common/service_interfaces/hostport.h>
#include <common/service_interfaces/service_factory.h>
#include <common/telemetry/log.h>

namespace uh::cluster {

template <typename service_interface>
class hostports_observer : public subscriber_observer {
public:
    using callback_t = std::function<void(hostport&)>;
    hostports_observer(
        const std::string& expected_parent_key,
        service_factory<service_interface> factory,
        std::initializer_list<
            std::reference_wrapper<service_observer<service_interface>>>
            observers,
        callback_t callback = nullptr,
        std::optional<std::size_t> ignore_id = std::nullopt)
        : m_expected_parent_key{expected_parent_key},
          m_ignore_id{ignore_id},
          m_service_factory{factory},
          m_observers{observers} {}

    hostports_observer(
        const std::string& expected_parent_key,
        service_factory<service_interface> factory,
        std::initializer_list<
            std::reference_wrapper<service_observer<service_interface>>>
            observers,
        std::size_t ignore_id)
        : m_expected_parent_key{expected_parent_key},
          m_ignore_id{ignore_id},
          m_service_factory{factory},
          m_observers{observers} {}
    /*
     * listener
     */
    bool on_watch(etcd_manager::response resp) {

        auto key = std::filesystem::path(resp.key);

        if (key.parent_path() != m_expected_parent_key)
            return false;

        auto id = deserialize<std::size_t>(key.filename().string());

        if (m_ignore_id.has_value() and id == m_ignore_id.value()) {
            LOG_INFO() << "Ignoring service with id " << id;
            return true;
        }

        switch (get_etcd_action_enum(resp.action)) {
        case etcd_action::GET:
        case etcd_action::CREATE:
            LOG_INFO() << std::format(
                "Connecting to service {} with id {} and hostport {}",
                get_service_string(service_interface::service_role), id,
                resp.value);
            add(id, deserialize<hostport>(resp.value));
            break;
        case etcd_action::SET:
            LOG_INFO() << std::format(
                "updating connection to service {} with id {} and hostport {}",
                get_service_string(service_interface::service_role), id,
                resp.value);
            remove(id);
            add(id, deserialize<hostport>(resp.value));
            break;
        case etcd_action::DELETE:
            LOG_INFO() << std::format(
                "Removing connection to service {} with id {} and hostport {}",
                get_service_string(service_interface::service_role), id,
                resp.value);
            remove(id);
            break;
        default:
            return false;
        }

        return true;
    }

    [[nodiscard]] std::size_t size() const noexcept {
        return m_client_count.load(std::memory_order_acquire);
    }

private:
    void add(std::size_t id, const hostport& hp) {
        auto it = m_clients.find(id);
        if (it != m_clients.end()) {
            return;
        }
        auto client = m_clients.emplace_hint(
            it, id, m_service_factory.make_service(hp.hostname, hp.port));

        for (auto& m : m_observers) {
            m.get().add_client(id, client->second);
        }
        m_client_count.fetch_add(1);
    }

    void remove(std::size_t id) {
        auto it = m_clients.find(id);
        if (it == m_clients.end()) {
            return;
        }

        LOG_DEBUG() << "remove callback for service "
                    << get_service_string(service_interface::service_role)
                    << ": " << id << " called. ";

        for (auto& m : m_observers) {
            m.get().remove_client(id);
        }
        m_clients.erase(it);
        m_client_count.fetch_sub(1);
    }

    std::string m_expected_parent_key;
    std::optional<std::size_t> m_ignore_id;
    service_factory<service_interface> m_service_factory;
    std::vector<std::reference_wrapper<service_observer<service_interface>>>
        m_observers;
    std::map<std::size_t, std::shared_ptr<service_interface>> m_clients;
    std::atomic<std::size_t> m_client_count{0};
};

} // namespace uh::cluster
