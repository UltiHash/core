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
        service_registry(std::string  service_id, const std::string etcd_host = "http://127.0.0.1:2379") :
            m_etcd_host(std::move(etcd_host)),
            m_service_id(std::move(service_id)),
            m_etcd_client(m_etcd_host)
        {
        }

        void register_service() {
            m_etcd_keepalive = m_etcd_client.leasekeepalive(m_etcd_default_ttl).get();
            std::string key = m_etcd_default_key_prefix + m_service_id;
            m_etcd_client.set(key, boost::asio::ip::host_name(), m_etcd_keepalive->Lease());
        }

        void unregister_service() {
            m_etcd_keepalive->Cancel();
        }

        std::vector<std::pair<std::size_t, std::string>> get_service_instances(uh::cluster::role service_role) {
            if(!abbreviation_by_role.contains(service_role))
                throw std::invalid_argument ("Invalid role!");
            std::vector<std::pair<std::size_t, std::string>> result;

            std::string service_key = "/uh/" + abbreviation_by_role.at(service_role);
            etcd::Response service_instances = m_etcd_client.ls(service_key).get();
            for (int i = 0; i < service_instances.keys().size(); i++) {
                const auto& service_instance = service_instances.value(i);
                std::filesystem::path service_path(service_instance.key());
                try {
                    std::size_t service_index = std::stoull(service_path.filename().string());
                    result.emplace_back(service_index, service_instance.as_string());
                } catch (std::invalid_argument& e) {
                    LOG_ERROR() << "Failed to extract instance id from key " << service_key << ".";
                    return result;
                }
            }

            return result;
        }

    private:
    #ifdef NDEBUG
        const std::size_t m_etcd_default_ttl = 5;
    #else
        const std::size_t m_etcd_default_ttl = 5;
    #endif

        const std::string m_etcd_default_key_prefix = "/uh/";

        const std::string m_etcd_host;
        const std::string m_service_id;

        etcd::Client m_etcd_client;
        std::shared_ptr<etcd::KeepAlive> m_etcd_keepalive;

    };

}

#endif //UH_CLUSTER_SERVICE_REGISTRY_H
