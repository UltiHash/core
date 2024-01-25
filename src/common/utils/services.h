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
        explicit services_index(config_registry&) {}
        void add(const std::size_t&, std::shared_ptr <client>) {}
        void erase(const std::size_t&) {}
    };

    template<>
    class services_index<STORAGE_SERVICE> {
    public:

        explicit services_index(config_registry& config_reg) :
            m_max_data_store_size(config_reg.get_global_data_view_config().max_data_store_size) {
        }

        void add(const std::size_t& id, std::shared_ptr <client> cl) {
            m_offsets.emplace(m_max_data_store_size * id, std::move(cl));
        }

        void erase(const std::size_t& id) {
            m_offsets.erase(m_max_data_store_size * id);
        }

        [[nodiscard]] std::shared_ptr <client> get(const uint128_t& pointer) const {

            const auto pfd = m_offsets.upper_bound (pointer);

            if (pfd == m_offsets.cbegin()) [[unlikely]] {
                throw std::out_of_range ("The pointer is not in the range of data nodes");
            }

            const auto n = std::prev (pfd);
            return n->second;
        }

    private:
        std::map < uint128_t, std::shared_ptr <client>> m_offsets;
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
            const auto service_prefix_path = etcd_services_attributes_key_prefix + get_service_string(r) + '/' + service_id + '/';

            const auto etcd_action = get_etcd_action_enum(response.action());
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
            return m_services_index.get(k);
        }

        [[nodiscard]] std::shared_ptr<client> get(const std::size_t& id) const {
            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);
            return m_clients.at(id);
        }

        [[nodiscard]] std::shared_ptr <client> get() const
        {
            auto index = m_nodes_index.load();

            std::shared_lock<std::shared_mutex> lk(m_shared_mutex);

            auto new_val = (index + 1) % m_clients.size();
            while (!m_nodes_index.compare_exchange_weak (index, new_val)) {
                index = m_nodes_index.load();
                new_val = (index + 1) % m_clients.size();
            }

            return m_clients.at(index);
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

            m_clients.emplace(service.id, cl);
            m_services_index.add(service.id, std::move(cl));
        }

        void wait_for_dependency() {
            const std::string dependency_key(etcd_services_announced_key_prefix + get_service_string(r));

            while(m_etcd_client.ls(dependency_key).get().keys().empty()) {
                LOG_INFO() << "waiting for dependency " << dependency_key << " to become available...";
                sleep(5);
            }
            LOG_INFO() << "dependency " << dependency_key << " seems to be available.";

            std::vector<service_endpoint> ds_instances = get_service_instances();

            for(const auto& instance : ds_instances) {
                add_service(instance);
            }
        }

    private:

        std::vector<service_endpoint> get_service_instances() {
            std::map<std::size_t, service_endpoint> endpoints_by_id;

            const std::string service_prefix_path(etcd_services_attributes_key_prefix + get_service_string(r) + "/");

            etcd::Response service_instances = m_etcd_client.ls(service_prefix_path).get();
            for (size_t i = 0; i < service_instances.keys().size(); i++) {

                const auto& service_instance = service_instances.value(i);
                std::string service_relative_path = service_instance.key().substr(service_prefix_path.length());

                std::size_t service_index = get_valid_index (service_relative_path.substr(0, service_relative_path.find('/')));
                auto [it, success] =
                        endpoints_by_id.insert(std::pair(std::size_t(service_index),
                                                         service_endpoint{.role = r, .id = service_index}));

                const std::string config_string(service_relative_path.substr(service_relative_path.rfind('/') + 1));
                if (config_string == get_config_string(uh::cluster::CFG_ENDPOINT_HOST)) {
                    it->second.host = service_instance.as_string();
                } else if (config_string == get_config_string(uh::cluster::CFG_ENDPOINT_PORT)) {
                    it->second.port = std::stoull(service_instance.as_string());
                }
            }

            std::vector<service_endpoint> result;
            result.reserve(endpoints_by_id.size());

            std::transform(endpoints_by_id.begin(), endpoints_by_id.end(), std::back_inserter(result),
                           [](const auto& pair) { return pair.second; });

            return result;
        }

        static size_t get_valid_index(const std::string& str) {
            size_t pos;
            const size_t num = std::stoull(str, &pos);
            if (pos != str.length()) {
                throw std::invalid_argument("Invalid service index detected.");
            }
            return num;
        }

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
