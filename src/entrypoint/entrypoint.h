#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>

#include "common/db/db.h"
#include "common/etcd/registry/service_id.h"
#include "common/etcd/registry/service_registry.h"
#include "config.h"
#include "deduplicator/deduplicator.h"
#include "entrypoint/directory.h"
#include "entrypoint/http/default_request_factory.h"
#include "entrypoint/limits.h"
#include "entrypoint_handler.h"

namespace uh::cluster {

class entrypoint {
public:
    explicit entrypoint(const service_config& sc, entrypoint_config config)
        : m_config(std::move(config)),
          m_ioc(boost::asio::io_context(m_config.server.threads)),

          m_etcd_client(sc.etcd_url),
          m_service_id(get_service_id(m_etcd_client,
                                      get_service_string(ENTRYPOINT_SERVICE),
                                      sc.working_dir)),
          m_service_registry(ENTRYPOINT_SERVICE, m_service_id, m_etcd_client),

          m_attached_storage(sc, m_config.m_attached_storage),
          m_attached_dedupe(sc, m_config.m_attached_deduplicator),
          m_storage_maintainer(
              m_etcd_client,
              service_factory<storage_interface>(
                  m_ioc,
                  m_config.global_data_view.storage_service_connection_count,
                  m_attached_storage.get_local_service_interface())),
          m_dedupe_maintainer(
              m_etcd_client,
              service_factory<deduplicator_interface>(
                  m_ioc, m_config.dedupe_node_connection_count,
                  m_attached_dedupe.get_local_service_interface())),
          m_data_view(m_config.global_data_view, m_ioc, m_storage_maintainer),

          m_directory(m_ioc, m_config.database),
          m_uploads(m_ioc, m_config.database),
          m_limits(sc.license.max_data_store_size),
          m_server(
              m_config.server,
              std::make_unique<entrypoint_handler>(
                  command_factory(m_ioc, m_dedupe_load_balancer, m_directory,
                                  m_uploads, m_config, m_data_view, m_limits),
                  std::make_unique<ep::http::default_request_factory>()),
              m_ioc) {
        m_dedupe_maintainer.add_monitor(m_dedupe_load_balancer);
    }

    void run() {
        m_registration =
            m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() {
        LOG_INFO() << "stopping " << m_service_registry.get_service_name();
        m_server.stop();
    }

    ~entrypoint() noexcept {
        m_dedupe_maintainer.remove_monitor(m_dedupe_load_balancer);
    }

private:
    entrypoint_config m_config;

    boost::asio::io_context m_ioc;
    etcd::SyncClient m_etcd_client;
    std::size_t m_service_id;
    service_registry m_service_registry;
    std::unique_ptr<service_registry::registration> m_registration;

    attached_service<storage> m_attached_storage;
    attached_service<deduplicator> m_attached_dedupe;

    service_maintainer<storage_interface> m_storage_maintainer;
    service_maintainer<deduplicator_interface> m_dedupe_maintainer;
    roundrobin_load_balancer<deduplicator_interface> m_dedupe_load_balancer;

    global_data_view m_data_view;
    directory m_directory;

    multipart_state m_uploads;
    limits m_limits;
    server m_server;
};

} // end namespace uh::cluster

#endif // CORE_ENTRY_NODE_H
