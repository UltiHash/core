
#ifndef UH_CLUSTER_SERVICE_MAINTAINER_H
#define UH_CLUSTER_SERVICE_MAINTAINER_H

#include "common/etcd/namespace.h"
#include "common/etcd/service_discovery/service_monitor.h"
#include "common/service_interfaces/service_factory.h"
#include "common/utils/time_utils.h"
#include <etcd/SyncClient.hpp>

namespace uh::cluster {

struct service_endpoint {
    std::size_t id{};
    std::map<etcd_service_attributes, std::string> attributes;
};

template <typename service_interface> struct service_maintainer {

    service_maintainer(etcd_manager& etcd,
                       service_factory<service_interface> service_factory)
        : m_service_factory(std::move(service_factory)) {

        etcd.watch(get_service_root_path(service_interface::service_role),
                   [this](const etcd::Response& response) {
                       return handle_state_changes(response);
                   });
        auto key_vals =
            etcd.ls(get_service_root_path(service_interface::service_role));

        for (auto& [key, val] : key_vals) {
            add(key, val);
        }
    }

    void add_monitor(service_monitor<service_interface>& monitor) {

        std::lock_guard l(m_mutex);
        for (const auto& [id, cl] : m_clients) {
            monitor.add_client(id, cl);
            for (const auto& se = m_detected_service_endpoints.at(id);
                 const auto& [attr_name, attr_val] : se.attributes) {
                monitor.add_attribute(cl, attr_name, attr_val);
            }
        }

        m_monitors.emplace_back(monitor);
    }

    void remove_monitor(service_monitor<service_interface>& monitor) {
        std::lock_guard l(m_mutex);
        m_monitors.remove_if(
            [&monitor](const service_monitor<service_interface>& m) {
                return &monitor == &m;
            });
    }

    [[nodiscard]] size_t size() const noexcept { return m_clients.size(); }

private:
    void handle_state_changes(const etcd::Response& response) {

        try {
            const auto& etcd_path = response.value().key();
            const auto& value = response.value().as_string();

            LOG_DEBUG() << "action: " << response.action()
                        << ", key: " << etcd_path << ", value: " << value;

            std::lock_guard<std::mutex> lk(m_mutex);

            switch (get_etcd_action_enum(response.action())) {
            case etcd_action::create:
                add(etcd_path, value);
                break;
            case etcd_action::set:
                set(etcd_path, value);
                break;
            case etcd_action::erase:
                remove(etcd_path, value);
                break;
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error while handling service state change: "
                       << e.what();
        }
    }

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
            cl != m_clients.cend() and attribute.has_value()) {
            for (auto& m : m_monitors) {
                m.get().add_attribute(cl->second, *attribute, value);
            }
        } else if (cl == m_clients.cend() and
                   itr->second.attributes.contains(ENDPOINT_HOST) and
                   itr->second.attributes.contains(ENDPOINT_PORT) and
                   itr->second.attributes.contains(ENDPOINT_PID)) {
            LOG_INFO() << "connecting to "
                       << itr->second.attributes.at(ENDPOINT_HOST) << ":"
                       << itr->second.attributes.at(ENDPOINT_PORT);

            auto client_itr = m_clients.emplace_hint(
                cl, itr->first,
                m_service_factory.make_service(
                    itr->second.attributes.at(ENDPOINT_HOST),
                    std::stoul(itr->second.attributes.at(ENDPOINT_PORT)),
                    std::stol(itr->second.attributes.at(ENDPOINT_PID))));

            for (auto& m : m_monitors) {

                m.get().add_client(client_itr->first, client_itr->second);

                for (const auto& [attr_name, attr_val] :
                     itr->second.attributes) {
                    m.get().add_attribute(client_itr->second, attr_name,
                                          attr_val);
                }
            }
        }
    }

    void set(const std::string& path, const std::string& value) {
        remove(path, value);
        add(path, value);
    }

    void remove(const std::string& path, const std::string&) {

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
            try {
                for (auto& m : m_monitors) {
                    m.get().remove_attribute(it->second, attr);
                }
            } catch (...) {
            }
        } else {

            LOG_DEBUG() << "remove callback for service "
                        << get_service_string(service_interface::service_role)
                        << ": " << id << " called. ";

            try {
                for (auto& m : m_monitors) {
                    m.get().remove_client(id, it->second);
                }
            } catch (...) {
            }
            m_detected_service_endpoints.erase(id);
            m_clients.erase(it);
        }
    }

    std::mutex m_mutex;
    std::map<std::size_t, std::shared_ptr<service_interface>> m_clients;
    std::map<std::size_t, service_endpoint> m_detected_service_endpoints;

    service_factory<service_interface> m_service_factory;
    std::list<std::reference_wrapper<service_monitor<service_interface>>>
        m_monitors;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_SERVICE_MAINTAINER_H
