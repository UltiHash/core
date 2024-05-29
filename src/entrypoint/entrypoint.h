#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>

#include "common/db/db.h"
#include "common/registry/service_id.h"
#include "common/registry/service_registry.h"
#include "config.h"
#include "deduplicator/deduplicator.h"
#include "entrypoint/directory.h"
#include "entrypoint_handler.h"

namespace uh::cluster {

class entrypoint {
public:
    explicit entrypoint(const service_config& sc,
                        const entrypoint_config& config)
        : m_etcd_client(sc.etcd_url),
          m_service_id(get_service_id(m_etcd_client,
                                      get_service_string(ENTRYPOINT_SERVICE),
                                      sc.working_dir)),
          m_ioc(boost::asio::io_context(config.server.threads)),
          m_attached_storage(sc, config.m_attached_storage),
          m_storage_services(
              m_etcd_client,
              service_factory<storage_interface>(
                  m_ioc,
                  config.global_data_view.storage_service_connection_count,
                  m_attached_storage.get_local_service_interface())),
          m_data_view(config.global_data_view, m_ioc, m_storage_services),
          m_max_data_size(sc.license.max_data_store_size),
          m_service_registry(ENTRYPOINT_SERVICE, m_service_id, m_etcd_client),
          m_db(config.database),
          m_config(config),
          m_attached_dedupe(sc, config.m_attached_deduplicator),
          m_dedupe_services(
              m_etcd_client,
              service_factory<deduplicator_interface>(
                  m_ioc, config.dedupe_node_connection_count,
                  m_attached_dedupe.get_local_service_interface())),
          m_directory(m_db),
          m_collection(get_reference_collection()),
          m_server(config.server, make_entrypoint_handler(m_collection),
                   m_ioc) {}

    void run() {
        m_registration =
            m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() {
        LOG_INFO() << "stopping " << m_service_registry.get_service_name();
        m_server.stop();
    }

private:
    reference_collection get_reference_collection() {
        return {.ioc = m_ioc,
                .dedupe_services = m_dedupe_services,
                .directory = m_directory,
                .server_state = m_state,
                .config = m_config,
                .gdv = m_data_view,
                .data_storage_size = m_data_storage_size,
                .max_data_size = m_max_data_size};
    }

    etcd::SyncClient m_etcd_client;
    std::size_t m_service_id;
    boost::asio::io_context m_ioc;
    attached_service<storage> m_attached_storage;
    services<storage_interface> m_storage_services;
    global_data_view m_data_view;
    std::atomic<std::size_t> m_data_storage_size = 0ull;
    std::size_t m_max_data_size = 0ull;

    service_registry m_service_registry;
    db::database m_db;

    entrypoint_config m_config;

    attached_service<deduplicator> m_attached_dedupe;

    services<deduplicator_interface> m_dedupe_services;
    directory m_directory;

    state m_state;

    reference_collection m_collection;
    server m_server;

    std::unique_ptr<service_registry::registration> m_registration;
};

} // end namespace uh::cluster

#endif // CORE_ENTRY_NODE_H
