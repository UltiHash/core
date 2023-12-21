//
// Created by max on 13.12.23.
//

#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include <string>
#include "third-party/etcd-cpp-apiv3/etcd/Client.hpp"
#include "third-party/etcd-cpp-apiv3/etcd/KeepAlive.hpp"
#include <boost/asio.hpp>

#include "common.h"
#include "log.h"

namespace uh::cluster {

    class service_registry {

    public:
        service_registry(std::string service_id, const std::string& etcd_host) :
            m_etcd_host(etcd_host),
            m_service_id(std::move(service_id)),
            m_etcd_client(m_etcd_host)
        {
        }

        void register_service(std::uint16_t port = 0) {
            m_etcd_keepalive = m_etcd_client.leasekeepalive(m_etcd_default_ttl).get();
            std::string key_base = m_etcd_default_key_prefix + m_service_id;
            std::string key_host = key_base + "/" + get_cfg_param_string(uh::cluster::CFG_HOST);
            std::string key_port = key_base + "/" + get_cfg_param_string(uh::cluster::CFG_PORT);
            m_etcd_client.set(key_host, boost::asio::ip::host_name(), m_etcd_keepalive->Lease());
            m_etcd_client.set(key_port, std::to_string(port), m_etcd_keepalive->Lease());
        }

        ~service_registry() {
            if(m_etcd_keepalive != NULL)
                m_etcd_keepalive->Cancel();
        }

        std::size_t get_config_value(const std::string& parameter) {
            //check if an instance specific setting is available
            //if not, check the global namespace
            return 0;
        }

        std::vector<service_endpoint> get_service_instances(uh::cluster::role service_role) {
            std::map<std::size_t, service_endpoint> endpoints_by_id;

            std::filesystem::path service_key(m_etcd_default_key_prefix + get_service_string(service_role));
            etcd::Response service_instances = m_etcd_client.ls(service_key.string()).get();
            for (int i = 0; i < service_instances.keys().size(); i++) {
                const auto& service_instance = service_instances.value(i);
                std::filesystem::path service_full_path(service_instance.key());
                std::filesystem::path service_rel_path = std::filesystem::relative(service_full_path, service_key);
                try {
                    std::string begin = service_rel_path.begin()->string();
                    std::size_t service_index = std::stoull(service_rel_path.begin()->string());
                    if(!endpoints_by_id.contains(service_index))
                        endpoints_by_id.emplace(std::size_t(service_index), service_endpoint{.role = service_role, .id = service_index});

                    auto &endpoint = endpoints_by_id.at(service_index);
                    if(service_rel_path.filename().string() == get_cfg_param_string(uh::cluster::CFG_HOST)) {
                        endpoint.host = service_instance.as_string();
                    } else if (service_rel_path.filename().string() == get_cfg_param_string(uh::cluster::CFG_PORT)) {
                        endpoint.port = std::stoul(service_instance.as_string());
                    }
                } catch (std::invalid_argument& e) {
                    LOG_ERROR() << "Failed to extract instance id from key " << service_key.string() << ".";
                    endpoints_by_id.clear();
                    break;
                }
            }

            std::vector<service_endpoint> result;
            result.reserve(endpoints_by_id.size());

            std::transform(endpoints_by_id.begin(), endpoints_by_id.end(), std::back_inserter(result),
                           [](const auto& pair) { return pair.second; });

            return result;
        }

        void wait_for_dependency(uh::cluster::role dependency) {
            std::string dependency_key = m_etcd_default_key_prefix + get_service_string(dependency);

            while(m_etcd_client.ls(dependency_key).get().keys().empty()) {
                LOG_INFO() << "Waiting for dependency " << dependency_key << " to become available...";
                sleep(5);
            }
            LOG_INFO() << "Dependency " << dependency_key << " seems to be available.";
        }

    private:
    #ifdef NDEBUG
        const std::size_t m_etcd_default_ttl = 5;
    #else
        // this value is used when running a debug build
        // having a longer ttl is important here as running into a breakpoint will almost
        // immediately let you run into ttl and thus will make debugging more complicated
        const std::size_t m_etcd_default_ttl = 60;
    #endif

        const std::string m_etcd_default_key_prefix = "/uh/";

        const std::string m_etcd_host;
        const std::string m_service_id;

        etcd::Client m_etcd_client;
        std::shared_ptr<etcd::KeepAlive> m_etcd_keepalive;

    };

}

#endif //UH_CLUSTER_SERVICE_REGISTRY_H
