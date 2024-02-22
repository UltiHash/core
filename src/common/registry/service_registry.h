#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include "etcd/KeepAlive.hpp"
#include "etcd/SyncClient.hpp"
#include <string>

#include "common/utils/cluster_config.h"

namespace uh::cluster {

class service_registry {

public:
    service_registry(uh::cluster::role role, std::size_t index,
                     etcd::SyncClient& etcd_client);

    [[nodiscard]] const std::string& get_service_name() const;

    class registration {
    public:
        registration(etcd::SyncClient& client,
                     const std::map<std::string, std::string>& kv_pairs,
                     std::size_t ttl);

        ~registration();

    private:
        etcd::SyncClient& m_client;
        int64_t m_lease;
        etcd::KeepAlive m_keepalive;
    };

    std::unique_ptr<registration> register_service(const server_config& config);

private:
    static constexpr std::size_t m_etcd_default_ttl = 30;

    const std::string m_service_name;
    etcd::SyncClient& m_etcd_client;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_SERVICE_REGISTRY_H
