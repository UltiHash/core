
#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include "common/registry/namespace.h"
#include "common/registry/service_id.h"
#include "common/utils/pointer_traits.h"
#include "common/utils/service_factory.h"
#include "common/utils/time_utils.h"
#include "storage/interfaces/storage_interface.h"
#include "third-party/etcd-cpp-apiv3/etcd/SyncClient.hpp"
#include "third-party/etcd-cpp-apiv3/etcd/Watcher.hpp"
#include <ranges>

namespace uh::cluster {

enum class etcd_action : uint8_t {
    create = 0,
    erase,
};

inline static etcd_action get_etcd_action_enum(const std::string& action_str) {
    static const std::map<std::string, etcd_action> etcd_action = {
        {"create", etcd_action::create},
        {"delete", etcd_action::erase},
    };

    if (etcd_action.contains(action_str))
        return etcd_action.at(action_str);
    else
        throw std::invalid_argument("invalid etcd action");
}

template <typename service_interface> class tmp_services_index {
public:
    void add(const std::size_t&, std::shared_ptr<service_interface>) {}
    void erase(const std::size_t&) {}
};

template <> class tmp_services_index<storage_interface> {
public:
    void add(const std::size_t& id, std::shared_ptr<storage_interface> cl) {
        m_clients.emplace(id, std::move(cl));
    }

    void erase(const std::size_t& id) { m_clients.erase(id); }

    [[nodiscard]] std::shared_ptr<storage_interface>
    get(const uint128_t& pointer) const {

        auto id = pointer_traits::get_service_id(pointer);
        return m_clients.at(id);
    }

private:
    std::map<uint32_t, std::shared_ptr<storage_interface>> m_clients;
};

template <typename service_interface> class services {
public:
    template <typename... index_args>
    services(etcd::SyncClient& etcd_client,
             service_factory<service_interface> service_factory,
             index_args... ia)
        : m_etcd_client(etcd_client),
          m_watcher(
              m_etcd_client,
              etcd_services_announced_key_prefix +
                  get_service_string(service_interface::service_role),
              [this](etcd::Response response) {
                  return handle_state_changes(response);
              },
              true),
          m_robin_index(m_clients.end()),
          m_services_index(ia...),
          m_service_factory(std::move(service_factory)) {
        auto path = etcd_services_announced_key_prefix +
                    get_service_string(service_interface::service_role);

        auto resp = wait_for_success(
            ETCD_TIMEOUT, ETCD_RETRY_INTERVAL,
            [this, &path]() { return m_etcd_client.ls(path); });
        for (const auto& key : resp.keys()) {
            add(key);
        }
    }

    ~services() { m_watcher.Cancel(); }

    template <typename key>
    std::shared_ptr<service_interface> get(key k) const {
        std::shared_ptr<service_interface> cl;

        std::unique_lock<std::shared_mutex> lk(m_mutex);
        if (m_cv.wait_for(
                lk, std::chrono::seconds(m_timeout_s), [this, &k, &cl]() {
                    try {
                        cl = m_services_index.get(k);
                        return true;
                    } catch (const std::out_of_range& range_exception) {
                        return false;
                    }
                })) {
        } else
            throw std::runtime_error("timeout waiting for client: " +
                                     k.to_string());

        return cl;
    }

    std::shared_ptr<service_interface> get(std::size_t id) const {
        std::shared_ptr<service_interface> cl;

        std::unique_lock<std::shared_mutex> lk(m_mutex);
        if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
                          [this, &id, &cl]() {
                              auto it = m_clients.find(id);

                              if (it == m_clients.end())
                                  return false;

                              cl = it->second;
                              return true;
                          })) {
        } else
            throw std::runtime_error("timeout waiting for client");

        return cl;
    }

    std::shared_ptr<service_interface> get() const {
        std::unique_lock<std::shared_mutex> lk(m_mutex);
        if (m_cv.wait_for(lk, std::chrono::seconds(m_timeout_s),
                          [this]() { return !m_clients.empty(); })) {
        } else
            throw std::runtime_error("timeout waiting for client");

        if (auto lc = m_service_factory.get_local_service(); lc) {
            return lc;
        }

        if (m_robin_index == m_clients.end()) {
            m_robin_index = m_clients.begin();
        }

        auto rv = m_robin_index->second;
        ++m_robin_index;

        return rv;
    }

    std::vector<std::shared_ptr<service_interface>> get_services() const {
        std::vector<std::shared_ptr<service_interface>> clients_list;

        std::shared_lock<std::shared_mutex> lk(m_mutex);
        clients_list.reserve(m_clients.size());
        std::ranges::copy(m_clients | std::views::values,
                          std::back_inserter(clients_list));

        return clients_list;
    }

private:
    void handle_state_changes(const etcd::Response& response) {
        LOG_DEBUG() << "action: " << response.action()
                    << ", key: " << response.value().key()
                    << ", value: " << response.value().as_string();

        try {
            const auto& etcd_path = response.value().key();
            const auto etcd_action = get_etcd_action_enum(response.action());

            switch (etcd_action) {
            case etcd_action::create:
                add(etcd_path);
                break;

            case etcd_action::erase:
                remove(etcd_path);
                break;
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error while handling service state change: "
                       << e.what();
        }
    }

    service_endpoint extract(const std::string& path) {

        const auto id = std::filesystem::path(path).filename().string();
        service_endpoint service_endpoint;
        service_endpoint.id = std::stoul(id);

        const std::string attributes_prefix(
            etcd_services_attributes_key_prefix +
            get_service_string(service_interface::service_role) + '/' + id +
            '/');

        const auto attributes = wait_for_success(
            ETCD_TIMEOUT, ETCD_RETRY_INTERVAL, [this, &attributes_prefix]() {
                return m_etcd_client.ls(attributes_prefix);
            });

        std::optional<std::string> host;
        std::optional<uint16_t> port;
        std::optional<int> pid;

        for (size_t i = 0; i < attributes.keys().size(); i++) {
            const auto attribute_name =
                std::filesystem::path(attributes.key(i)).filename().string();

            if (attribute_name == get_config_string(CFG_ENDPOINT_HOST)) {
                host = attributes.value(i).as_string();
            } else if (attribute_name == get_config_string(CFG_ENDPOINT_PORT)) {
                port = std::stoul(attributes.value(i).as_string());
            } else if (attribute_name == get_config_string(CFG_ENDPOINT_PID)) {
                pid = std::stol(attributes.value(i).as_string());
            }
        }

        if (!host || !port || !pid) {
            throw std::runtime_error("client not available");
        }

        service_endpoint.port = *port;
        service_endpoint.host = *host;
        service_endpoint.pid = *pid;
        return service_endpoint;
    }

    void add(const std::string& path) {
        const auto service_endpoint = extract(path);

        LOG_DEBUG() << "add callback for service "
                    << get_service_string(service_interface::service_role)
                    << ": " << service_endpoint.id
                    << " called. host: " << service_endpoint.host
                    << " port: " << service_endpoint.port;

        std::unique_lock<std::shared_mutex> lk(m_mutex);
        if (m_clients.contains(service_endpoint.id)) [[unlikely]]
            return;

        auto cl = m_service_factory.make_service(service_endpoint);

        m_clients.emplace(service_endpoint.id, cl);
        m_services_index.add(service_endpoint.id, std::move(cl));
        lk.unlock();
        m_cv.notify_one();
    }

    void remove(const std::string& path) {
        const auto id =
            std::stoull(std::filesystem::path(path).filename().string());

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

        m_services_index.erase(id);
    }

    etcd::SyncClient& m_etcd_client;
    etcd::Watcher m_watcher;

    mutable std::shared_mutex m_mutex;
    mutable std::condition_variable_any m_cv;
    std::map<std::size_t, std::shared_ptr<service_interface>> m_clients;

    mutable std::map<std::size_t,
                     std::shared_ptr<service_interface>>::const_iterator
        m_robin_index;
    tmp_services_index<service_interface> m_services_index;
    service_factory<service_interface> m_service_factory;
    static constexpr std::size_t m_timeout_s = 10;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_SERVICES_H
