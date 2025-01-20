#pragma once

#include "common/etcd/registry/service_id.h"
#include "common/etcd/registry/service_registry.h"
#include "common/global_data/default_global_data_view.h"
#include "common/network/server.h"
#include "common/service_interfaces/attached_service.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "common/telemetry/log.h"
#include "config.h"
#include "handler.h"
#include "storage/service.h"
#include <functional>
#include <utility>

namespace uh::cluster::deduplicator {

class service {
public:
    explicit service(const service_config& sc,
                     const deduplicator_config& config)
        : m_ioc(boost::asio::io_context(config.server.threads)),
          m_etcd{sc.etcd_config},
          m_service_id(get_service_id(m_etcd,
                                      get_service_string(DEDUPLICATOR_SERVICE),
                                      sc.working_dir)),
          m_service_registry(DEDUPLICATOR_SERVICE, m_service_id, m_etcd),
          m_attached_storage(sc, config.m_attached_storage),
          m_storage_maintainer(
              m_etcd,
              service_factory<storage_interface>(
                  m_ioc,
                  config.global_data_view.storage_service_connection_count,
                  m_attached_storage.get_local_service_interface())),
          m_data_view(config.global_data_view, m_ioc, m_storage_maintainer,
                      m_etcd),
          m_deduplicator(
              std::make_shared<local_deduplicator>(config, m_data_view)),
          m_server(config.server, std::make_unique<handler>(*m_deduplicator),
                   m_ioc) {}

    void run() {
        m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() { m_server.stop(); }

    std::shared_ptr<local_deduplicator> get_local_interface() {
        return m_deduplicator;
    }

    size_t id() const noexcept { return m_service_id; }

private:
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    std::size_t m_service_id;

    service_registry m_service_registry;

    attached_service<storage::service> m_attached_storage;
    service_maintainer<storage_interface> m_storage_maintainer;

    default_global_data_view m_data_view;
    std::shared_ptr<local_deduplicator> m_deduplicator;
    server m_server;
};
} // namespace uh::cluster::deduplicator
