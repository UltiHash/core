#ifndef UH_CLUSTER_SERVICE_MAINTAINER_H
#define UH_CLUSTER_SERVICE_MAINTAINER_H

#include <ranges>
#include <utility>

#include "log.h"
#include "common/network/client.h"
#include "common/utils/service_registry.h"
#include "entrypoint/rest_server.h"

namespace uh::cluster {

    class services {
    public:

        services (const role r, service_registry& registry, const std::shared_ptr<boost::asio::io_context>& ioc,
                  const std::uint16_t port, const int connection_count):
        m_registry(registry), m_ioc(ioc), m_role(r), m_port(port), m_connection_count(connection_count) {
            m_registry.register_callback_add_service(m_role, [this](const service_info& service) { add_service_callback(service); });
            m_registry.register_callback_remove_service(m_role, [this](const service_info& service) { remove_service_callback(service); });
        }

        std::shared_ptr <client> get(const std::size_t id) const {
            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);

            auto map_iterator = m_clients.find(id);

            if (map_iterator == m_clients.end()) {
                return nullptr;
            } else {
                return map_iterator->second;
            }
        }

        [[nodiscard]] std::vector <std::shared_ptr <client>> get_clients_list() const {
            std::vector <std::shared_ptr <client>> clients_list;

            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);
            clients_list.reserve(m_clients.size());
            std::ranges::copy(m_clients | std::views::values, std::back_inserter(clients_list));

            return clients_list;
        }

    private:
        mutable std::shared_mutex m_shared_mutex;
        std::map <size_t, std::shared_ptr <client>> m_clients;

        service_registry& m_registry;
        const std::shared_ptr<boost::asio::io_context>& m_ioc;
        const role m_role;
        const std::uint16_t m_port;
        const int m_connection_count;

        void add_service_callback(const service_info& service) {

            LOG_INFO() << "add callback for service " << get_service_string(m_role) << " called.";

            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);
            m_clients.insert({service.id, std::make_shared<client>(m_ioc, service.value,
                                                                   m_port,
                                                                   m_connection_count)});
        }

        void remove_service_callback(const service_info& service) {
            LOG_INFO() << "removed callback for service " << get_service_string(m_role) << " called.";

            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);
            m_clients.erase(service.id);
        }
    };

    class datanode_services {
    public:

        datanode_services (const role r, service_registry& registry, boost::asio::io_context& ioc,
                           const std::uint16_t port, const int connection_count):
                m_registry(registry), m_ioc(ioc), m_role(r), m_port(port), m_connection_count(connection_count) {
            m_registry.register_callback_add_service(m_role, [this](const service_info& service) { add_service_callback(service); });
            m_registry.register_callback_remove_service(m_role, [this](const service_info& service) { remove_service_callback(service); });
        }

        inline auto size() const noexcept {
            return m_data_node_offsets.size();
        }

        std::shared_ptr <client> get(const std::size_t id) const {
            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);

            auto map_iterator = m_clients.find(id);

            if (map_iterator == m_clients.end()) {
                return nullptr;
            } else {
                return map_iterator->second;
            }
        }

        std::shared_ptr <client> get(const uint128_t) const {
            // logic of upper bound goes here
        }

        [[nodiscard]] std::vector <std::shared_ptr <client>> get_clients_list() const {

            std::vector <std::shared_ptr <client>> clients_list;

            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);
            clients_list.reserve(m_clients.size());
            std::ranges::copy(m_clients | std::views::values, std::back_inserter(clients_list));

            return clients_list;
        }

    private:
        mutable std::shared_mutex m_shared_mutex;
        std::map <const size_t, std::shared_ptr <client>> m_clients;
        std::map <const uint128_t, std::shared_ptr <client>> m_data_node_offsets;

        service_registry& m_registry;
        boost::asio::io_context& m_ioc;
        const role m_role;
        const std::uint16_t m_port;
        const int m_connection_count;

        void add_service_callback(const service_info& service) {
            LOG_INFO() << "add callback for service " << get_service_string(m_role) << " called.";

            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);

            auto cl = std::make_shared<client>(m_ioc, service.value,
                                               m_port,
                                               m_connection_count);

            m_clients.insert({service.id, cl});
            const uint128_t offset = make_storage_config().max_data_store_size * (service.id);
            m_data_node_offsets.emplace(offset, std::move(cl));
        }

        void remove_service_callback(const service_info& service) {
            LOG_INFO() << "remove callback for service " << get_service_string(m_role) << " called.";

            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);

            m_clients.erase(service.id);
            const uint128_t offset = make_storage_config().max_data_store_size * (service.id);
            m_data_node_offsets.erase(offset);
        }
    };

} // uh::cluster

#endif // UH_CLUSTER_SERVICE_MAINTAINER_H