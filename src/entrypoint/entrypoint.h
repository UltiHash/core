#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>

#include "common/registry/service_id.h"
#include "common/registry/service_registry.h"
#include "common/registry/services.h"
#include "config.h"
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
          m_service_registry(ENTRYPOINT_SERVICE, m_service_id, m_etcd_client),
          m_config(config),
          m_dedupe_services(m_ioc, config.dedupe_node_connection_count,
                            m_etcd_client),
          m_directory_services(m_ioc, config.directory_connection_count,
                               m_etcd_client),
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
                .directory_services = m_directory_services,
                .server_state = m_state,
                .config = m_config};
    }

    etcd::SyncClient m_etcd_client;
    std::size_t m_service_id;
    boost::asio::io_context m_ioc;

    service_registry m_service_registry;

    entrypoint_config m_config;

    services<DEDUPLICATOR_SERVICE, coro_client> m_dedupe_services;
    services<DIRECTORY_SERVICE, coro_client> m_directory_services;
    state m_state;

    reference_collection m_collection;
    server m_server;

    std::unique_ptr<service_registry::registration> m_registration;
};

} // end namespace uh::cluster

#endif // CORE_ENTRY_NODE_H
