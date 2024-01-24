#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include <shared_mutex>
#include <ranges>
#include <utility>

#include "log.h"
#include "common/network/client.h"
#include "common/registry/config_registry.h"
#include "etcd/Watcher.hpp"

namespace uh::cluster {

    enum class etcd_action : uint8_t {
        create = 0,
        erase,
    };

    static etcd_action get_etcd_action_enum(const std::string &action_str) {
        static const std::map<std::string, etcd_action> etcd_action = {
                {"create", etcd_action::create},
                {"delete", etcd_action::erase},
        };

        if (etcd_action.contains(action_str))
            return etcd_action.at(action_str);
        else
            throw std::invalid_argument("invalid etcd action");
    }

    template<role r>
    class services_index {
    public:
        void erase(const uint128_t& offset) {};
        void add(const uint128_t& offset, std::shared_ptr <client> client) {}
    };

    template<>
    class services_index<STORAGE_SERVICE> {
    public:

        [[nodiscard]] std::shared_ptr <client> get(const uint128_t& offset) const {

            const auto pfd = m_data_node_offsets.upper_bound (offset);

            if (pfd == m_data_node_offsets.cbegin()) [[unlikely]] {
                throw std::out_of_range ("The pointer is not in the range of data nodes");
            }
            const auto n = std::prev (pfd);
            return n->second;
        }

        void erase(const uint128_t& offset) {
            m_data_node_offsets.erase(offset);
        }

        void add(const uint128_t& offset, std::shared_ptr <client> client) {
            m_data_node_offsets.emplace(offset, std::move(client));
        }

    private:
        std::map <const uint128_t, std::shared_ptr <client>> m_data_node_offsets;
    };

    template <role r>
    class services {
    public:

        services(boost::asio::io_context& ioc, config_registry& config_registry, const int connection_count, std::string etcd_host):
                m_ioc(ioc),
                // How can we pass config_registry only and try to get the connection count according to the role
                m_connection_count(connection_count),
                m_etcd_client(etcd_host),
                m_watcher(etcd_host, etcd_services_announced_key_prefix + get_service_string(r), [this](etcd::Response response) {return handle_state_changes(response);}, true),
                m_config_registry(config_registry)
        {}

        ~services() {
            // joins all the watcher thread
            m_watcher.Cancel();
        }

        void handle_state_changes(const etcd::Response& response)
        {
            LOG_DEBUG() << "action: " << response.action() << ", key: " << response.value().key() << ", value: " << response.value().as_string();

            const auto& key = response.value().key();

            const std::string service_id = std::filesystem::path(key).filename().string();
            const auto etcd_action = get_etcd_action_enum(response.action());
            const auto service_prefix_path = etcd_services_attributes_key_prefix + get_service_string(r) + '/' + service_id + '/';

            switch (etcd_action) {
                case etcd_action::create:

                    add_service_callback({.role = r,
                                          .id = std::stoul(service_id),
                                          .host = m_etcd_client.get(service_prefix_path + get_config_string(uh::cluster::CFG_ENDPOINT_HOST))
                                                  .get().value().as_string(),
                                          .port = static_cast<uint16_t>(std::stoul(m_etcd_client.get(service_prefix_path + get_config_string(uh::cluster::CFG_ENDPOINT_PORT))
                                                  .get().value().as_string()))});
                    break;

                case etcd_action::erase:
                    remove_service_callback({.role = r,
                                              .id = std::stoul(service_id)
                                            });
                    break;
            }

        }

        [[nodiscard]] std::shared_ptr <client> get(const uint128_t& offset) const {
            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);
            return m_services_index.get(offset);
        }

        [[nodiscard]] std::shared_ptr <client> get(const std::size_t& id) const {
            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);

            auto map_iterator = m_clients.find(id);

            if (map_iterator == m_clients.end()) {
                return nullptr;
            } else {
                return map_iterator->second;
            }
        }

        [[nodiscard]] std::shared_ptr <client> get() const
        {
            auto index = m_nodes_index.load();

            const auto services = get_clients();
            const auto services_size = services.size();

            auto new_val = (index + 1) % services_size;
            while (!m_nodes_index.compare_exchange_weak (index, new_val)) {
                index = m_nodes_index.load();
                new_val = (index + 1) % services_size;
            }

            return services.at(index);
        }

        [[nodiscard]] std::vector <std::shared_ptr <client>> get_clients() const {
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

            auto max_storage_size = m_config_registry.get_global_data_view_config().max_data_store_size;
            m_services_index.add(service.id * max_storage_size, std::move(cl));
        }

        [[nodiscard]] inline role get_role() const noexcept {
            return r;
        }

    private:

        boost::asio::io_context& m_ioc;
        const int m_connection_count;
        etcd::Client m_etcd_client;
        etcd::Watcher m_watcher;

        mutable std::shared_mutex m_shared_mutex;
        std::map <size_t, std::shared_ptr <client>> m_clients;

        mutable std::atomic <size_t> m_nodes_index {};
        services_index<r> m_services_index;

        config_registry& m_config_registry;

        void add_service_callback(const service_endpoint& service) {

            LOG_INFO() << "add callback for service " << service.role << " called.";

            add_service(service);
        }

        void remove_service_callback(const service_endpoint& service) {
            LOG_INFO() << "removed callback for service " << service.role << " called.";

            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);
            m_clients.erase(service.id);
            auto max_storage_size = m_config_registry.get_global_data_view_config().max_data_store_size;
            m_services_index.erase(service.id * max_storage_size);
        }

    };

} // end namespace uh::cluster

#endif // UH_CLUSTER_SERVICES_H
