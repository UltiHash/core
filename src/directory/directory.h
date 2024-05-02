#ifndef CORE_DIRECTORY_NODE_H
#define CORE_DIRECTORY_NODE_H

#include "common/license/license.h"
#include "common/registry/attached_service.h"
#include "common/registry/service_id.h"
#include "directory/interfaces/directory_interface.h"
#include "directory_handler.h"
#include <common/telemetry/log.h>
#include <functional>

namespace uh::cluster {

class directory {
public:
    explicit directory(const service_config& sc, const directory_config& config)
        : m_etcd_client(sc.etcd_url),
          m_service_id(get_service_id(m_etcd_client,
                                      get_service_string(DIRECTORY_SERVICE),
                                      sc.working_dir)),
          m_ioc(boost::asio::io_context(config.server.threads)),
          m_service_registry(DIRECTORY_SERVICE, m_service_id, m_etcd_client),
          m_attached_storage(sc, config.m_attached_storage),
          m_storage_services(
              m_etcd_client,
              service_factory<storage_interface>(
                  m_ioc,
                  config.global_data_view.storage_service_connection_count,
                  m_attached_storage.get_local_service_interface())),
          m_data_view(config.global_data_view, m_ioc, m_storage_services),
          m_directory(std::make_shared<local_directory>(config, m_data_view)),
          m_server(config.server,
                   std::make_unique<directory_handler>(*m_directory), m_ioc) {}

    void run() {
        m_registration =
            m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() { m_server.stop(); }

    std::shared_ptr<local_directory> get_local_interface() {
        return m_directory;
    }

private:
    etcd::SyncClient m_etcd_client;
    std::size_t m_service_id;
    boost::asio::io_context m_ioc;
    service_registry m_service_registry;

    attached_service<storage> m_attached_storage;

    services<storage_interface> m_storage_services;

    global_data_view m_data_view;
    std::shared_ptr<local_directory> m_directory;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};

} // end namespace uh::cluster

#endif // CORE_DIRECTORY_NODE_H
