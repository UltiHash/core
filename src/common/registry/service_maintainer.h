
#ifndef UH_CLUSTER_SERVICE_MAINTAINER_H
#define UH_CLUSTER_SERVICE_MAINTAINER_H

#include "common/registry/namespace.h"
#include "common/registry/service_id.h"
#include "common/utils/service_factory.h"
#include "third-party/etcd-cpp-apiv3/etcd/SyncClient.hpp"
#include "third-party/etcd-cpp-apiv3/etcd/Watcher.hpp"

namespace uh::cluster {

enum class etcd_action : uint8_t {
    create = 0,
    set,
    erase,
};

inline static etcd_action get_etcd_action_enum(const std::string& action_str) {
    static const std::map<std::string, etcd_action> etcd_action = {
        {"create", etcd_action::create},
        {"set", etcd_action::set},
        {"delete", etcd_action::erase},
    };

    if (etcd_action.contains(action_str))
        return etcd_action.at(action_str);
    else
        throw std::invalid_argument("invalid etcd action");
}

struct service_endpoint {
    std::size_t id{};
    std::map<etcd_service_attributes, std::string> attributes;
};

template <typename service_interface>
struct service_maintainer {
    service_maintainer(etcd::SyncClient& etcd_client,
             service_factory<service_interface> service_factory)
        : m_etcd_client(etcd_client),
          m_watcher(
              m_etcd_client,
              get_service_root_path(service_interface::service_role),
              [this](etcd::Response response) {
                  return handle_state_changes(response);
              },
              true),
          m_robin_index(m_clients.end()),
          m_service_factory(std::move(service_factory)),
          m_local_service (m_service_factory.get_local_service()) {

        auto resp = wait_for_success(
            ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
            [this]() { return m_etcd_client.ls(get_service_root_path(service_interface::service_role)); });
        auto vals = resp.values();
        auto keys = resp.keys();
        for (size_t i = 0; i < vals.size(); i++) {
            add(keys[i], vals[i].as_string());
        }
    }
    ~service_maintainer() { m_watcher.Cancel(); }

protected:
    void handle_state_changes(const etcd::Response& response) {

        try {
            const auto& etcd_path = response.value().key();
            const auto& value = response.value().as_string();


            LOG_DEBUG() << "action: " << response.action()
                        << ", key: " << etcd_path
                        << ", value: " << value;

            const auto etcd_action = get_etcd_action_enum(response.action());
            switch (etcd_action) {
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
        std::lock_guard<std::shared_mutex> lk(m_mutex);
        const auto id = get_id(path);
        auto itr = m_detected_service_endpoints.find(id);
        if (itr == m_detected_service_endpoints.cend()) {
            service_endpoint se {.id = id};
            itr = m_detected_service_endpoints.emplace_hint(itr, id, se);
        }
        LOG_INFO() << "New ETCD entry " << id;
        if (service_attributes_path(path)) {
            itr->second.attributes.insert_or_assign(get_etcd_key_enum(get_attribute_key(path)), value);
            LOG_INFO() << "ETCD attribute entry " << get_attribute_key(path) << " " << value;
        }

        if (!m_clients.contains(id) and
            itr->second.attributes.contains(ENDPOINT_HOST) and
            itr->second.attributes.contains(ENDPOINT_PORT) and
            itr->second.attributes.contains(ENDPOINT_PID)) {
            LOG_INFO() << "connecting to " << itr->second.attributes.at(ENDPOINT_HOST) << ":"
                       << itr->second.attributes.at(ENDPOINT_PORT);
            m_clients.emplace(itr->first, m_service_factory.make_service(
                                              itr->second.attributes.at(ENDPOINT_HOST),
                                              std::stoul(itr->second.attributes.at(ENDPOINT_PORT)),
                                              std::stol(itr->second.attributes.at(ENDPOINT_PID))
                                              ));
        }

        m_cv.notify_one();
    }

    void set(const std::string& path, const std::string& value) {
        std::lock_guard<std::shared_mutex> lk(m_mutex);
        const auto id = get_id(path);
        if (service_attributes_path(path)) {
            auto& se = m_detected_service_endpoints.at(id);
            se.attributes.insert_or_assign(get_etcd_key_enum(get_attribute_key(path)), value);
            LOG_DEBUG() << "New ETCD attribute entry " << get_attribute_key(path)
                        << ": " << value
                        << " for endpoint "
                        << se.attributes.at(ENDPOINT_HOST)
                        << ":" << se.attributes.at(ENDPOINT_PORT);
        }
    }

    void remove(const std::string& path, const std::string& value) {

        const auto id = get_id(path);

        if (service_attributes_path(path)) {
            m_detected_service_endpoints.at(id).attributes.erase(get_etcd_key_enum(value));
            return;
        }

        LOG_DEBUG() << "remove callback for service "
                    << get_service_string(service_interface::service_role)
                    << ": " << id << " called. ";

        std::unique_lock<std::shared_mutex> lk(m_mutex);
        auto it = m_clients.find(id);
        if (it == m_clients.end()) {
            return;
        }

        if (it == m_robin_index) {
            m_robin_index = m_clients.erase(it);
        } else {
            m_clients.erase(it);
        }

    }

    etcd::SyncClient& m_etcd_client;
    etcd::Watcher m_watcher;

    mutable std::shared_mutex m_mutex;
    mutable std::condition_variable_any m_cv;
    std::map<std::size_t, std::shared_ptr<service_interface>> m_clients;
    std::map<std::size_t, service_endpoint> m_detected_service_endpoints;
    mutable std::map<std::size_t,
                     std::shared_ptr<service_interface>>::const_iterator
        m_robin_index;
    service_factory<service_interface> m_service_factory;
    std::shared_ptr<service_interface> m_local_service;
};

}

#endif // UH_CLUSTER_SERVICE_MAINTAINER_H
