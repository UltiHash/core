//
// Created by max on 13.12.23.
//

#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include <string>
#include <boost/asio.hpp>
#include "etcd/Client.hpp"
#include "etcd/KeepAlive.hpp"
#include "etcd/v3/Transaction.hpp"

#include "common/utils/common.h"
#include "common/utils/cluster_config.h"
#include "common/utils/log.h"
#include "namespace.h"
#include "etcd/v3/Transaction.hpp"

namespace uh::cluster {

    class service_registry {

    public:
        service_registry(uh::cluster::role role, std::size_t index, const std::string& etcd_host) :
                m_service_name(get_service_string(role) + "/" + std::to_string(index)),
                m_etcd_client(etcd_host)
        {
        }

        [[nodiscard]] const std::string& get_service_name() const {
            return m_service_name;
        }

        class registration
        {
        public:
            registration(etcd::Client& client, const std::map<std::string, std::string>& kv_pairs, std::size_t ttl)
                : m_client(client),
                  m_lease(m_client.leasegrant(ttl).get().value().lease()),
                  m_keepalive(m_client, ttl, m_lease)
            {
                etcdv3::Transaction txn;
                for(const auto& pair : kv_pairs)
                    txn.add_success_put(pair.first, pair.second, m_lease);
                m_client.txn(txn).get();
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

            const std::string announced_key_base = etcd_services_announced_key_prefix + m_service_name;

            const std::string key_base = etcd_services_attributes_key_prefix + m_service_name + "/";

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

    private:

        static constexpr std::size_t m_etcd_default_ttl = 10;

        const std::string m_etcd_host;
        const std::string m_service_name;

        etcd::Client m_etcd_client;
    };

}

#endif //UH_CLUSTER_SERVICE_REGISTRY_H
