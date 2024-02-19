#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <atomic>
#include <functional>
#include <iostream>
#include <utility>

#include "common/network/server.h"
#include "common/registry/config_registry.h"
#include "common/registry/service_registry.h"
#include "common/utils/cluster_config.h"
#include "data_store.h"
#include "storage_handler.h"

namespace uh::cluster {

class storage {
  public:
    explicit storage(const service_config& sc)
        : m_etcd_client(sc.etcd_url),
          m_config_registry(STORAGE_SERVICE, m_etcd_client, sc.working_dir),
          m_ioc(m_config_registry.get_server_config().threads),
          m_service_registry(STORAGE_SERVICE,
                             m_config_registry.get_service_id(), m_etcd_client),
          m_server(m_config_registry.get_server_config(),
                   m_service_registry.get_service_name(),
                   std::make_unique<storage_handler>(
                       m_config_registry.get_storage_config(),
                       m_config_registry.get_service_id()),
                   m_ioc) {}

    void run() {
        m_registration =
            m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() { m_server.stop(); }

    ~storage() {
        LOG_DEBUG() << "terminating " << m_service_registry.get_service_name();
    }

  private:
    etcd::SyncClient m_etcd_client;
    config_registry m_config_registry;
    boost::asio::io_context m_ioc;
    service_registry m_service_registry;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};
} // end namespace uh::cluster
#endif // CORE_DATA_NODE_H
