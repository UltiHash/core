#ifndef CORE_DIRECTORY_NODE_H
#define CORE_DIRECTORY_NODE_H

#include "common/license/license.h"
#include "common/utils/cluster_config.h"
#include "directory_handler.h"
#include <common/utils/log.h>
#include <functional>

namespace uh::cluster {

directory_config update_config(directory_config conf, const license& license) {
    conf.max_data_store_size = license.max_data_store_size;
    return conf;
}

class directory {
  public:
    explicit directory(const service_config& sc)
        : m_etcd_client(sc.etcd_url),
          m_config_registry(DIRECTORY_SERVICE, m_etcd_client, sc.working_dir),
          m_ioc(boost::asio::io_context(
              m_config_registry.get_server_config().threads)),
          m_service_registry(DIRECTORY_SERVICE,
                             m_config_registry.get_service_id(), m_etcd_client),
          m_storage_services(m_ioc, m_config_registry,
                             m_config_registry.get_global_data_view_config()
                                 .storage_service_connection_count,
                             m_etcd_client),
          m_config(update_config(m_config_registry.get_directory_config(),
                                 sc.license)),
          m_directory_workers(std::make_shared<boost::asio::thread_pool>(
              m_config.worker_thread_count)),
          m_storage(m_config_registry.get_global_data_view_config(), m_ioc,
                    m_storage_services),
          m_server(m_config_registry.get_server_config(),
                   m_config_registry.get_service_name(),
                   std::make_unique<directory_handler>(m_config, m_storage,
                                                       m_directory_workers),
                   m_ioc) {}

    void run() {
        m_registration =
            m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() {
        m_server.stop();
        m_directory_workers->join();
        m_directory_workers->stop();
    }

  private:
    etcd::SyncClient m_etcd_client;
    config_registry m_config_registry;
    boost::asio::io_context m_ioc;
    service_registry m_service_registry;

    services<STORAGE_SERVICE> m_storage_services;

    directory_config m_config;
    std::shared_ptr<boost::asio::thread_pool> m_directory_workers;
    global_data_view m_storage;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};

} // end namespace uh::cluster

#endif // CORE_DIRECTORY_NODE_H
