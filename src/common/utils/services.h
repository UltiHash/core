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

        services(boost::asio::io_context& ioc,
                 config_registry& config_registry,
                 const int connection_count,
                 std::string etcd_host) :
                 m_ioc(ioc),
                 m_connection_count(connection_count),
                 m_etcd_client(etcd_host),
                 m_watcher(etcd_host, etcd_services_announced_key_prefix + get_service_string(r),
                          [this](etcd::Response response) {return handle_state_changes(response);}, true),
                 m_services_index(config_registry)
        {
            wait_for_dependency();
        }

        ~services() {
            m_watcher.Cancel();
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

    private:

        void handle_state_changes(const etcd::Response& response)
        {
            LOG_DEBUG() << "action: " << response.action() << ", key: " << response.value().key()
                        << ", value: " << response.value().as_string();

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

        }

        void wait_for_dependency() {
            const std::string dependency_key(etcd_services_announced_key_prefix + get_service_string(r));

            if(m_etcd_client.ls(dependency_key).get().keys().empty()) {
                LOG_INFO() << "waiting for dependency " << dependency_key << " to become available...";
                std::unique_lock<std::shared_mutex> lk(m_shared_mutex);
                m_cv.wait(lk, [this]() { return !m_clients.empty(); });
            } else {
                add_service_instances();
            }

            LOG_INFO() << "dependency " << dependency_key << " seems to be available.";
        }

        void add_service_instances() {
            const std::string service_prefix_path(etcd_services_announced_key_prefix + get_service_string(r) + "/");

            etcd::Response service_instances = m_etcd_client.ls(service_prefix_path).get();

            for (size_t i = 0; i < service_instances.keys().size(); i++) {
                add(service_instances.key(i));
            }
        }

        service_endpoint extract(const std::string& path) const {

            const auto id = std::filesystem::path(path).filename().string();
            service_endpoint service_endpoint;
            service_endpoint.id = std::stoul(id);

            const std::string attributes_prefix(etcd_services_attributes_key_prefix +
                                                get_service_string(r) + '/' + id  + '/');
            etcd::Response attributes = m_etcd_client.ls(attributes_prefix).get();

            for (size_t i = 0; i < attributes.keys().size(); i++) {
                const auto attribute_name = std::filesystem::path(attributes.key(i))
                                            .filename().string();

                if (attribute_name == get_config_string(CFG_ENDPOINT_HOST)) {
                    service_endpoint.host = attributes.value(i).as_string();
                } else if (attribute_name == get_config_string(CFG_ENDPOINT_PORT)) {
                    service_endpoint.port = std::stoul(attributes.value(i).as_string());
                }
            }

            return service_endpoint;
        }

        void add(const std::string& path) {
            const auto service_endpoint = extract(path);

            LOG_INFO() << "add callback for service " << get_service_string(r) << ": "
                        << service_endpoint.id << " called. host: " << service_endpoint.host << " port: "
                        << service_endpoint.port ;

            std::unique_lock<std::shared_mutex> lk(m_shared_mutex);
            if (m_clients.contains(service_endpoint.id)) [[unlikely]]
                return;

            auto cl = std::make_shared<client>(m_ioc, service_endpoint.host,
                                               service_endpoint.port,
                                               m_connection_count);

            m_clients.emplace(service_endpoint.id, cl);
            m_services_index.add(service_endpoint.id, std::move(cl));
            lk.unlock();
            m_cv.notify_one();
        }

        void remove(const std::string& path) {
            const auto service_endpoint = extract(path);

            LOG_INFO() << "remove callback for service " << get_service_string(r) << ": "
                       << service_endpoint.id << " called. host: " << service_endpoint.host << " port: "
                       << service_endpoint.port ;

            std::lock_guard<std::shared_mutex> shared_lk(m_shared_mutex);
            m_clients.erase(service_endpoint.id);
            m_services_index.erase(service_endpoint.id);
        }

        boost::asio::io_context& m_ioc;
        const int m_connection_count;
        mutable etcd::Client m_etcd_client;
        etcd::Watcher m_watcher;

        mutable std::shared_mutex m_shared_mutex;
        std::condition_variable_any m_cv;
        std::map <std::size_t, std::shared_ptr <client>> m_clients;

        mutable std::atomic <size_t> m_nodes_index {};
        services_index<r> m_services_index;

    };

} // end namespace uh::cluster

#endif // UH_CLUSTER_SERVICES_H
