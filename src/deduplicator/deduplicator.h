#ifndef CORE_DEDUPE_NODE_H
#define CORE_DEDUPE_NODE_H

#include "common/global_data/global_data_view.h"
#include "common/network/server.h"
#include "common/registry/service_id.h"
#include "common/registry/service_registry.h"
#include "common/telemetry/log.h"
#include "common/utils/cluster_config.h"
#include "deduplicator_handler.h"
#include <functional>
#include <iostream>
#include <utility>

namespace uh::cluster {
class deduplicator {
public:
    explicit deduplicator(const service_config& sc)
        : m_etcd_client(sc.etcd_url),
          m_service_id(get_service_id(m_etcd_client,
                                      get_service_string(DEDUPLICATOR_SERVICE),
                                      sc.working_dir)),
          m_config_registry(DEDUPLICATOR_SERVICE, m_etcd_client, sc.working_dir,
                            m_service_id),
          m_ioc(boost::asio::io_context(
              m_config_registry.get_server_config().threads)),
          m_service_registry(DEDUPLICATOR_SERVICE, m_service_id, m_etcd_client),
          m_storage_services(m_ioc, m_config_registry,
                             m_config_registry.get_global_data_view_config()
                                 .storage_service_connection_count,
                             m_etcd_client),
          m_config(m_config_registry.get_deduplicator_config()),
          m_dedupe_workers(std::make_shared<boost::asio::thread_pool>(
              m_config.worker_thread_count)),
          m_storage(m_config_registry.get_global_data_view_config(), m_ioc,
                    m_storage_services),
          m_server(m_config_registry.get_server_config(),
                   std::make_unique<deduplicator_handler>(m_config, m_storage,
                                                          m_dedupe_workers),
                   m_ioc) {}

    void run() {
        m_registration =
            m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() {
        m_server.stop();
        m_dedupe_workers->join();
        m_dedupe_workers->stop();
    }

    ~deduplicator() {
        LOG_DEBUG() << "terminating " << m_service_registry.get_service_name();
        m_ioc.stop();
    }

private:
    etcd::SyncClient m_etcd_client;
    std::size_t m_service_id;
    config_registry m_config_registry;
    boost::asio::io_context m_ioc;

    service_registry m_service_registry;

    services<STORAGE_SERVICE> m_storage_services;

    deduplicator_config m_config;
    std::shared_ptr<boost::asio::thread_pool> m_dedupe_workers;

    global_data_view m_storage;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};
} // end namespace uh::cluster

#endif // CORE_DEDUPE_NODE_H
