#ifndef UH_CLUSTER_SERVICE_REGISTRY_H
#define UH_CLUSTER_SERVICE_REGISTRY_H

#include "common/etcd/namespace.h"
#include "common/network/server.h"
#include "etcd/KeepAlive.hpp"
#include "etcd/SyncClient.hpp"
#include <string>

namespace uh::cluster {

template <size_t I> struct test_dest {
    test_dest() = default;
    ~test_dest() { LOG_INFO() << "test destructor " << I; }
};

class service_registry {

public:
    service_registry(uh::cluster::role role, std::size_t index,
                     etcd::SyncClient& etcd_client);

    [[nodiscard]] std::string get_service_name() const;

    class registration {
    public:
        registration(etcd::SyncClient& client, role role, std::size_t index,
                     const std::map<std::string, std::string>& kv_pairs,
                     std::size_t ttl);

        void monitor(etcd_service_attributes key,
                     const std::function<std::string()>& func);

        ~registration();

    private:
        etcd::SyncClient& m_client;
        int64_t m_lease;
        etcd::KeepAlive m_keepalive;

        const role m_service_role;
        const size_t m_id;
        bool m_stop = false;
        test_dest<3> m_3;
        std::map<std::string, std::function<std::string()>>
            m_monitored_attributes;
        std::mutex m_attributes_mutex;
        test_dest<2> m_2;
        std::thread m_monitor_thread;
        test_dest<1> m_1;
        std::condition_variable m_cv;
        test_dest<0> m_0;
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
