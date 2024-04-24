#ifndef CORE_DIRECTORY_NODE_H
#define CORE_DIRECTORY_NODE_H

#include "common/license/license.h"
#include "common/registry/service_id.h"
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
          m_storage_services(
              m_etcd_client,
              std::make_unique<storage_factory> (m_ioc, config.global_data_view.storage_service_connection_count, get_local_storage ())),
          m_directory_workers(m_ioc, config.worker_thread_count),
          m_storage(config.global_data_view, m_ioc,
                    m_storage_services),
          m_server(config.server,
                   std::make_unique<directory_handler>(config, m_storage,
                                                       m_directory_workers),
                   m_ioc) {}

    void run() {
        if (m_attached_storage) {
            m_local_storage_thread = std::thread ([this] {m_attached_storage->run();});
        }

        m_registration =
            m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() {
        if (m_attached_storage) {
            m_attached_storage->stop();
            m_local_storage_thread.join();
        }
        m_server.stop();
    }

private:

    std::shared_ptr <local_storage> get_local_storage () {
        if (m_attached_storage) {
            return m_attached_storage->get_storage_interface();
        }
        return nullptr;
    }

    etcd::SyncClient m_etcd_client;
    std::size_t m_service_id;
    boost::asio::io_context m_ioc;
    service_registry m_service_registry;

    std::optional <storage> m_attached_storage;
    std::thread m_local_storage_thread;

    tmp_services<storage_interface> m_storage_services;

    worker_pool m_directory_workers;
    global_data_view m_storage;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};

} // end namespace uh::cluster

#endif // CORE_DIRECTORY_NODE_H
