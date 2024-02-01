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
                 m_robin_index(m_clients.end()),
                 m_services_index(config_registry)
        {
            auto path = etcd_services_announced_key_prefix + get_service_string(r);

            auto resp = m_etcd_client.ls(path);
            for (const auto& key : resp.keys()) {
                add(key);
            }
        }

        ~services() {
            m_watcher.Cancel();
        }

        template<typename key>
        std::shared_ptr <client> get(key k) const {
            std::shared_lock<std::shared_mutex> lk(m_mutex);
            return m_services_index.get(k);
        }

        std::shared_ptr<client> get(std::size_t id) const {
            std::shared_lock<std::shared_mutex> lk(m_mutex);

            auto it = m_clients.find(id);
            if (it == m_clients.end()) {
                throw std::runtime_error("no client with id " + std::to_string(id));
            }

            return it->second;
        }

        std::shared_ptr <client> get() const
        {
            std::unique_lock<std::shared_mutex> lk(m_mutex);
            return lockfree_get();
        }

        std::vector <std::shared_ptr <client>> get_clients() const {
            std::vector <std::shared_ptr <client>> clients_list;

            std::shared_lock<std::shared_mutex> lk(m_mutex);
            clients_list.reserve(m_clients.size());
            std::ranges::copy(m_clients | std::views::values, std::back_inserter(clients_list));

            return clients_list;
        }

        std::shared_ptr<client> wait() {
            std::unique_lock<std::shared_mutex> lock(m_mutex);
            m_cv.wait(lock, [this](){ return !m_clients.empty(); });

            return lockfree_get();
        }

    private:
        std::shared_ptr<client> lockfree_get() const {
            if (m_clients.empty()) {
                throw std::runtime_error("no client available");
            }

            if (m_robin_index == m_clients.end()) {
                m_robin_index = m_clients.begin();
            }

            auto rv = m_robin_index->second;
            ++m_robin_index;

            return rv;
        }

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

        service_endpoint extract(const std::string& path) {

            const auto id = std::filesystem::path(path).filename().string();
            service_endpoint service_endpoint;
            service_endpoint.id = std::stoul(id);

            const std::string attributes_prefix(etcd_services_attributes_key_prefix +
                                                get_service_string(r) + '/' + id  + '/');
            etcd::Response attributes = m_etcd_client.ls(attributes_prefix);

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

            std::unique_lock<std::shared_mutex> lk(m_mutex);
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

            std::unique_lock<std::shared_mutex> lk(m_mutex);
            auto it = m_clients.find(service_endpoint.id);
            if (it == m_clients.end())
            {
                return;
            }

            if (it == m_robin_index) {
                m_robin_index = m_clients.erase(it);
            } else {
                m_clients.erase(it);
            }

            m_services_index.erase(service_endpoint.id);
        }

        boost::asio::io_context& m_ioc;
        const int m_connection_count;
        etcd::SyncClient m_etcd_client;
        etcd::Watcher m_watcher;

        mutable std::shared_mutex m_mutex;
        std::condition_variable_any m_cv;
        std::map <std::size_t, std::shared_ptr <client>> m_clients;

        mutable std::map<std::size_t, std::shared_ptr<client>>::const_iterator m_robin_index;
        services_index<r> m_services_index;
    };

} // end namespace uh::cluster

#endif // UH_CLUSTER_SERVICES_H
