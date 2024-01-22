#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include <shared_mutex>
#include <ranges>
#include <utility>

#include "log.h"
#include "common/network/client.h"
#include "common/registry/service_registry.h"

namespace uh::cluster {

    class services {
    public:

        services (const role r, service_registry& registry, boost::asio::io_context& ioc, const int connection_count):
                  m_registry(registry),
                  m_ioc(ioc),
                  m_role(r),
                  m_connection_count(connection_count) {

            m_registry.register_callback_add_service(m_role, [this](const service_endpoint& service) { add_service_callback(service); });
            m_registry.register_callback_remove_service(m_role, [this](const service_endpoint& service) { remove_service_callback(service); });
        }

        std::shared_ptr <client> get()
        {
            auto index = m_nodes_index.load();

            const auto services = get_clients_list();
            const auto services_size = services.size();

            auto new_val = (index + 1) % services_size;
            while (!m_nodes_index.compare_exchange_weak (index, new_val)) {
                index = m_nodes_index.load();
                new_val = (index + 1) % services_size;
            }

            return services.at(index);
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

        void add_service(const service_endpoint& service) {
            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);

            if (m_clients.contains(service.id)) [[unlikely]]
                return;

            m_clients.insert({service.id, std::make_shared<client>(m_ioc, service.host,
                                                                   service.port,
                                                                   m_connection_count)});
        }

        [[nodiscard]] inline const role get_role() const noexcept {
            return m_role;
        }

    private:
        mutable std::shared_mutex m_shared_mutex;
        std::map <size_t, std::shared_ptr <client>> m_clients;

        std::atomic <size_t> m_nodes_index {};

        service_registry& m_registry;
        boost::asio::io_context& m_ioc;
        const role m_role;
        const int m_connection_count;

        void add_service_callback(const service_endpoint& service) {

            LOG_INFO() << "add callback for service " << get_service_string(m_role) << " called.";

            add_service(service);
        }

        void remove_service_callback(const service_endpoint& service) {
            LOG_INFO() << "removed callback for service " << get_service_string(m_role) << " called.";

            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);
            m_clients.erase(service.id);
        }
    };

    class datanode_services {
    public:

        datanode_services (const role r, service_registry& registry, boost::asio::io_context& ioc,
                           const int connection_count):
                            m_registry(registry),
                            m_ioc(ioc),
                            m_role(r),
                            m_connection_count(connection_count) {

            m_registry.register_callback_add_service(m_role, [this](const service_endpoint& service) { add_service_callback(service); });
            m_registry.register_callback_remove_service(m_role, [this](const service_endpoint& service) { remove_service_callback(service); });
        }

        std::shared_ptr <client> get() {
            auto index = m_data_node_index.load();

            // TODO: optimize this
            const auto services = get_clients_list();
            const auto services_size = services.size();

            auto new_val = (index + 1) % services_size;
            while (!m_data_node_index.compare_exchange_weak (index, new_val)) {
                index = m_data_node_index.load();
                new_val = (index + 1) % services_size;
            }

            return m_clients.at(index);
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

        // Question: What happens if we have 1 2 3 4 data index and we remove 2.
        std::shared_ptr <client> get(const uint128_t& pointer) const {
            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);

            const auto pfd = m_data_node_offsets.upper_bound (pointer);

            if (pfd == m_data_node_offsets.cbegin()) [[unlikely]] {
                throw std::out_of_range ("The pointer is not in the range of data nodes");
            }
            const auto n = std::prev (pfd);
            return n->second;
        }

        [[nodiscard]] std::vector <std::shared_ptr <client>> get_clients_list() const {
            std::vector <std::shared_ptr <client>> clients_list;

            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);
            clients_list.reserve(m_clients.size());
            std::ranges::copy(m_clients | std::views::values, std::back_inserter(clients_list));

            return clients_list;
        }

        void add_service(const service_endpoint& service) {
            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);

            if (m_clients.contains(service.id)) [[unlikely]]
                return;

            auto cl = std::make_shared<client>(m_ioc, service.host,
                                               service.port,
                                               m_connection_count);

            m_clients.insert({service.id, cl});
            const uint128_t offset = make_storage_config().max_data_store_size * (service.id);
            m_data_node_offsets.emplace(offset, std::move(cl));
        }

    private:
        mutable std::shared_mutex m_shared_mutex;
        std::map <const size_t, std::shared_ptr <client>> m_clients;
        std::map <const uint128_t, std::shared_ptr <client>> m_data_node_offsets;
        std::atomic <size_t> m_data_node_index {};

        service_registry& m_registry;
        boost::asio::io_context& m_ioc;
        const role m_role;
        const int m_connection_count;

        void add_service_callback(const service_endpoint& service) {
            LOG_INFO() << "add callback for service " << get_service_string(m_role) << " called.";

            add_service(service);
        }

        void remove_service_callback(const service_endpoint& service) {
            LOG_INFO() << "remove callback for service " << get_service_string(m_role) << " called.";

            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);

            m_clients.erase(service.id);
            const uint128_t offset = make_storage_config().max_data_store_size * (service.id);
            m_data_node_offsets.erase(offset);
        }
    };

} // end namespace uh::cluster

#endif // UH_CLUSTER_SERVICES_H
