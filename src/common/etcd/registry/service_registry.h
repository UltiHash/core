#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include "common/etcd/namespace.h"
#include "common/network/server.h"
#include "etcd/KeepAlive.hpp"
#include "etcd/SyncClient.hpp"
#include <string>

namespace uh::cluster {

class service_registry {

public:
    service_registry(uh::cluster::role role, std::size_t index,
                     etcd::SyncClient& etcd_client);

    [[nodiscard]] std::string get_service_name() const;

    class registration {
    public:
        registration(etcd::SyncClient& client, role role, std::size_t index,
                     std::map<std::string, std::string> kv_pairs,
                     std::size_t ttl);

        registration(const registration&) = delete;
        registration(registration&&) noexcept = delete;
        registration& operator=(const registration&) = delete;
        registration& operator=(registration&&) = delete;

        void monitor(etcd_service_attributes key,
                     const std::function<std::string()>& func);

        ~registration();

    private:
        struct etcd_handle {

            etcd_handle(etcd::SyncClient& client, std::size_t ttl,
                        const std::map<std::string, std::string>& kv_pairs);

            ~etcd_handle();
            void check();

            etcd::SyncClient& client;
            int64_t lease;
            etcd::KeepAlive keepalive;
        };

        etcd::SyncClient& m_client;
        std::map<std::string, std::string> m_kv_pairs;
        std::size_t m_ttl;

        const role m_service_role;
        const size_t m_id;

        std::unique_ptr<etcd_handle> m_handle;

        bool m_stop = false;
        std::map<std::string, std::function<std::string()>>
            m_monitored_attributes;
        std::condition_variable m_cv;
        std::mutex m_attributes_mutex;
        std::thread m_monitor_thread;
    };

    std::unique_ptr<registration> register_service(const server_config& config);

private:
    static constexpr std::size_t m_etcd_default_ttl = 30;

    const role m_service_role;
    const size_t m_id;
    etcd::SyncClient& m_etcd_client;
};

} // namespace uh::cluster

#endif // UH_CLUSTER_SERVICE_REGISTRY_H
