#ifndef UH_CLUSTER_SERVICES_H
#define UH_CLUSTER_SERVICES_H

#include <shared_mutex>
#include <ranges>
#include <utility>
#include <set>

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
        explicit services_index(config_registry& config_reg) {}

        void add(const std::size_t& id) {
            m_service_index.insert(id);
        }

        void erase(const std::size_t& id) {
            m_service_index.erase(id);
        }

        [[nodiscard]] std::size_t get(const std::size_t& id) const {
            auto set_iterator = m_service_index.find(id);
            if (set_iterator == m_service_index.end()) {
                throw std::out_of_range ("The pointer is not in the range of data nodes");
            } else {
                return id;
            }
        }

    private:
        std::set<std::size_t> m_service_index;
    };

    template<>
    class services_index<STORAGE_SERVICE> {
    public:

        explicit services_index(config_registry& config_reg) : m_max_data_store_size(config_reg.get_global_data_view_config().max_data_store_size) {
        }

        void add(const std::size_t& id) {
            m_storage_service_offsets.emplace(m_max_data_store_size * id, id);
        }

        void erase(const std::size_t& id) {
            m_storage_service_offsets.erase(id);
        }

        [[nodiscard]] std::size_t get(const uint128_t& offset) const {

            const auto pfd = m_storage_service_offsets.upper_bound (offset);

            if (pfd == m_storage_service_offsets.cbegin()) [[unlikely]] {
                throw std::out_of_range ("The pointer is not in the range of data nodes");
            }

            const auto n = std::prev (pfd);
            return n->second;
        }

        [[nodiscard]] std::size_t get(const std::size_t& id) const {
            auto offset = m_max_data_store_size * id;

            auto map_iterator = m_storage_service_offsets.find(offset);
            if (map_iterator == m_storage_service_offsets.end()) {
                throw std::out_of_range ("The pointer is not in the range of data nodes");
            } else {
                return id;
            }
        }

    private:
        std::map < const uint128_t, const std::size_t> m_storage_service_offsets;
        const uint128_t m_max_data_store_size;
    };

    template <role r>
    class services {
    public:

        services(boost::asio::io_context& ioc, config_registry& config_registry, const int connection_count, std::string etcd_host):
                m_ioc(ioc),
                m_connection_count(connection_count),
                m_etcd_client(etcd_host),
                m_watcher(etcd_host, etcd_services_announced_key_prefix + get_service_string(r), [this](etcd::Response response) {return handle_state_changes(response);}, true),
                m_services_index(config_registry)
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

        template<typename key>
        std::shared_ptr <client> get(key k) const {
            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);
            return m_clients.at(m_services_index.get(k));
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
            m_services_index.add(service.id);
        }

    private:

        boost::asio::io_context& m_ioc;
        const int m_connection_count;
        etcd::Client m_etcd_client;
        etcd::Watcher m_watcher;

        mutable std::shared_mutex m_shared_mutex;
        std::map <std::size_t, std::shared_ptr <client>> m_clients;

        mutable std::atomic <size_t> m_nodes_index {};
        services_index<r> m_services_index;

        void add_service_callback(const service_endpoint& service) {

            LOG_INFO() << "add callback for service " << service.role << " called.";

            add_service(service);
        }

        void remove_service_callback(const service_endpoint& service) {
            LOG_INFO() << "removed callback for service " << service.role << " called.";

            std::lock_guard<std::shared_mutex> lk(m_shared_mutex);
            m_clients.erase(service.id);
            m_services_index.erase(service.id);
        }

    };

} // end namespace uh::cluster

#endif // UH_CLUSTER_SERVICES_H
