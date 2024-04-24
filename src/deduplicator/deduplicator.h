#ifndef CORE_DEDUPE_NODE_H
#define CORE_DEDUPE_NODE_H

#include "common/global_data/global_data_view.h"
#include "common/network/server.h"
#include "common/registry/service_id.h"
#include "common/registry/service_registry.h"
#include "common/telemetry/log.h"
#include "config.h"
#include "deduplicator_handler.h"
#include "storage/storage.h"
#include "storage/tmp_services.h"
#include <functional>
#include <iostream>
#include <utility>

namespace uh::cluster {

class deduplicator {
public:
    explicit deduplicator(const service_config& sc,
                          const deduplicator_config& config)
        : m_etcd_client(sc.etcd_url),
          m_service_id(get_service_id(m_etcd_client,
                                      get_service_string(DEDUPLICATOR_SERVICE),
                                      sc.working_dir)),
          m_ioc(boost::asio::io_context(config.server.threads)),
          m_service_registry(DEDUPLICATOR_SERVICE, m_service_id, m_etcd_client),
          m_storage_services(m_etcd_client, std::make_unique<storage_factory> (m_ioc, config.global_data_view.storage_service_connection_count, get_local_storage ())),
          m_dedupe_workers(m_ioc, config.worker_thread_count),
          m_storage(config.global_data_view, m_ioc,
                    m_storage_services),
          m_server(config.server,
                   std::make_unique<deduplicator_handler>(config, m_storage,
                                                          m_dedupe_workers),
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

    ~deduplicator() {
        LOG_DEBUG() << "terminating " << m_service_registry.get_service_name();
        m_ioc.stop();
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

    worker_pool m_dedupe_workers;

    global_data_view m_storage;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;

};
} // end namespace uh::cluster

#endif // CORE_DEDUPE_NODE_H
