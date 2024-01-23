//
// Created by max on 13.12.23.
//

#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include <string>
#include "third-party/etcd-cpp-apiv3/etcd/Client.hpp"
#include "third-party/etcd-cpp-apiv3/etcd/KeepAlive.hpp"
#include <boost/asio.hpp>

#include "common/utils/common.h"
#include "common/utils/cluster_config.h"
#include "common/utils/log.h"
#include "namespace.h"
#include "etcd/v3/Transaction.hpp"

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

    class service_registry {

    public:
        service_registry(uh::cluster::role role, std::size_t index, std::string etcd_host) :
                m_etcd_host(std::move(etcd_host)),
                m_service_index(index),
                m_service_name(get_service_string(role) + "/" + std::to_string(index)),
                m_etcd_client(m_etcd_host)
        {
        }

        [[nodiscard]] const std::string& get_service_name() const {
            return m_service_name;
        }

        [[nodiscard]] std::size_t get_service_index() const {
            return m_service_index;
        }

        class registration
        {
        public:
            registration(etcd::Client& client, const std::map<std::string, std::string>& kv_pairs, std::size_t ttl)
                : m_client(client),
                  m_lease(m_client.leasegrant(ttl).get().value().lease()),
                  m_keepalive(m_client, ttl, m_lease)
            {
                for(const auto& pair : kv_pairs)
                    m_client.set(pair.first, pair.second, m_lease); // TODO: introduce transaction in this loop so that all the information is written to etcd at once
            }

            ~registration()
            {
                m_client.leaserevoke(m_lease);
            }

            private:
            etcd::Client& m_client;
            int64_t m_lease;
            etcd::KeepAlive m_keepalive;
        };

        std::unique_ptr<registration> register_service(const server_config& config) {

            // expose the announced
            const std::string announced_key_base = m_etcd_services_announced_key_prefix + m_service_name;

            const std::string key_base = m_etcd_services_attributes_key_prefix + m_service_name + "/";
            const std::map<std::string, std::string> kv_pairs =
                    {
                        {key_base + get_config_string(uh::cluster::CFG_ENDPOINT_HOST), boost::asio::ip::host_name()},
                        {key_base + get_config_string(uh::cluster::CFG_ENDPOINT_PORT),std::to_string(config.port)},
                        {announced_key_base , {}},
                    };

            return std::make_unique<registration>(
                m_etcd_client,
                kv_pairs,
                m_etcd_default_ttl);
        }

        std::vector<service_endpoint> get_service_instances(uh::cluster::role service_role) {
            std::map<std::size_t, service_endpoint> endpoints_by_id;

            // extract
            const std::string service_prefix_path(m_etcd_services_attributes_key_prefix + get_service_string(service_role) + "/");
            etcd::Response service_instances = m_etcd_client.ls(service_prefix_path).get();
            for (size_t i = 0; i < service_instances.keys().size(); i++) {

                // extract by key - get service endpoint struct
                const auto& service_instance = service_instances.value(i);
                std::string service_relative_path = service_instance.key().substr(service_prefix_path.length());

                std::size_t service_index = get_valid_index (service_relative_path.substr(0, service_relative_path.find('/')));
                auto [it, success] =
                        endpoints_by_id.insert(std::pair(std::size_t(service_index),
                                                            service_endpoint{.role = service_role, .id = service_index}));

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

        void wait_for_dependency(uh::cluster::role dependency) {
            const std::string dependency_key(m_etcd_services_announced_key_prefix + get_service_string(dependency));
            while(m_etcd_client.ls(dependency_key).get().keys().empty()) {
                LOG_INFO() << "waiting for dependency " << dependency_key << " to become available...";
                sleep(5);
            }
            LOG_INFO() << "dependency " << dependency_key << " seems to be available.";
        }

    private:
        static size_t get_valid_index(const std::string& str) {
                size_t pos;
                const size_t num = std::stoull(str, &pos);
                if (pos != str.length()) {
                    throw std::invalid_argument("Invalid service index detected.");
                }
                return num;

        }

        static constexpr std::size_t m_etcd_default_ttl = 10;

        const std::string m_etcd_host;
        const std::size_t m_service_index;
        const std::string m_service_name;

        etcd::Client m_etcd_client;
    };

}

#endif //UH_CLUSTER_SERVICE_REGISTRY_H
