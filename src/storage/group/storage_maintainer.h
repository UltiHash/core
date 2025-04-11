#pragma once

#include <common/etcd/service_discovery/service_observer.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <storage/interfaces/storage_group.h>

#include <vector>

namespace uh::cluster {

struct service_endpoint {
    std::size_t id{};
    std::map<etcd_service_attributes, std::string> attributes;
};

template class storage_maintainer {

public:
    template <typename... Observers>
    storage_maintainer(etcd_manager& etcd, Observers&... observers)
        : m_observers{std::ref(observers)...},
          m_watch_guard{etcd.watch(get_storage_group_state_path(),
                                   [this](etcd_manager::response resp) {
                                       return handle_state_changes(resp);
                                   })} {
        static_assert(
            (std::is_base_of_v<service_observer<storage_group>, Observers> &&
             ...),
            "All observers must be derived from BaseObserver");
    }

    [[nodiscard]] size_t size() const noexcept { return m_clients.size(); }

private:
    void handle_state_changes(etcd_manager::response resp) {

        try {
            const auto& etcd_path = resp.key;

            LOG_DEBUG() << "action: " << resp.action << ", key: " << etcd_path;

            switch (get_etcd_action_enum(resp.action)) {
            case etcd_action::GET:
            case etcd_action::CREATE: {
                LOG_DEBUG() << "value: " << resp.value;
                // TODO: Add storage group to the listeners if there's ec
                // enabled group.
                // TODO: Add storage interface if it's non-ec group.
                // add(etcd_path, resp.value);
                break;
            }
            case etcd_action::SET: {
                LOG_DEBUG() << "value: " << resp.value;
                set(etcd_path, resp.value);
                break;
            }
            case etcd_action::DELETE: {
                remove(etcd_path);
                break;
            }
            default:
                LOG_WARN() << "invalid etcd action: " << resp.action;
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error while handling service state change: "
                       << e.what();
        }
    }

    // TODO: Check if this function is multi-thread safe, since it is called by
    // etcd watcher.
    void add(const std::string& path, const std::string& value) {
        const auto id = get_id(path);

        auto itr = m_detected_service_endpoints.find(id);
        if (itr == m_detected_service_endpoints.cend()) {
            service_endpoint se{.id = id};
            itr = m_detected_service_endpoints.emplace_hint(itr, id, se);
        }

        std::optional<etcd_service_attributes> attribute;
        if (service_attributes_path(path)) {
            attribute.emplace(
                get_etcd_service_attribute_enum(get_attribute_key(path)));
            itr->second.attributes.insert_or_assign(*attribute, value);
        }

        if (auto cl = m_clients.find(id);
            cl == m_clients.cend() and
            itr->second.attributes.contains(ENDPOINT_HOST) and
            itr->second.attributes.contains(ENDPOINT_PORT)) {
            LOG_INFO() << "connecting to "
                       << itr->second.attributes.at(ENDPOINT_HOST) << ":"
                       << itr->second.attributes.at(ENDPOINT_PORT);

            // auto client_itr = m_clients.emplace_hint(
            //     cl, itr->first,
            //     m_service_factory.make_service(
            //         itr->second.attributes.at(ENDPOINT_HOST),
            //         std::stoul(itr->second.attributes.at(ENDPOINT_PORT))));

            // for (auto& m : m_observers) {
            //     m.get().add_client(client_itr->first, client_itr->second);
            // }
        }
    }

    void set(const std::string& path, const std::string& value) {
        remove(path);
        add(path, value);
    }

    void remove(const std::string& path) {

        const auto id = get_id(path);

        auto it = m_clients.find(id);
        if (it == m_clients.end()) {
            return;
        }

        if (service_attributes_path(path)) {
            auto attr =
                get_etcd_service_attribute_enum(get_attribute_key(path));
            try {
                m_detected_service_endpoints.at(id).attributes.erase(attr);
            } catch (...) {
            }
        } else {
            try {
                for (auto& m : m_observers) {
                    m.get().remove_client(id, it->second);
                }
            } catch (...) {
            }
            m_detected_service_endpoints.erase(id);
            m_clients.erase(it);
        }
    }

    std::map<std::size_t, std::shared_ptr<storage_group>> m_clients;
    std::map<std::size_t, service_endpoint> m_detected_service_endpoints;

    std::vector<std::reference_wrapper<service_observer<storage_group>>>
        m_observers;
    etcd_manager::watch_guard m_watch_guard;
};

} // namespace uh::cluster
