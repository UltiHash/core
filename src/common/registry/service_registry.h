#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include "common/network/server.h"
#include "common/utils/common.h"
#include "etcd/KeepAlive.hpp"
#include "etcd/SyncClient.hpp"
#include <string>

namespace uh::cluster {

struct service_config {
    std::string etcd_url = "http://127.0.0.1:2379";
    std::filesystem::path working_dir = "/var/lib/uh";
    std::string register_addr;
};

class service_registry {

public:
    service_registry(uh::cluster::role role, std::size_t index,
                     const std::string& register_addr,
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
    const std::string m_register_addr;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_SERVICE_REGISTRY_H
