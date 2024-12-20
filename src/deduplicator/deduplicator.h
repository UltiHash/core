#ifndef CORE_DEDUPE_NODE_H
#define CORE_DEDUPE_NODE_H

#include "common/etcd/registry/service_id.h"
#include "common/etcd/registry/service_registry.h"
#include "common/global_data/concrete_global_data_view.h"
#include "common/network/server.h"
#include "common/service_interfaces/attached_service.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "common/telemetry/log.h"
#include "config.h"
#include "deduplicator_handler.h"
#include "storage/storage.h"
#include <functional>
#include <utility>

namespace uh::cluster {

class deduplicator {
public:
    explicit deduplicator(etcd_manager& etcd, const service_config& sc,
                          const deduplicator_config& config)
        : m_service_id(get_service_id(
              etcd, get_service_string(DEDUPLICATOR_SERVICE), sc.working_dir)),
          m_ioc(boost::asio::io_context(config.server.threads)),
          m_service_registry(DEDUPLICATOR_SERVICE, m_service_id, etcd),
          m_attached_storage(etcd, sc, config.m_attached_storage),
          m_storage_maintainer(
              etcd,
              service_factory<storage_interface>(
                  m_ioc,
                  config.global_data_view.storage_service_connection_count,
                  m_attached_storage.get_local_service_interface())),
          m_data_view(config.global_data_view, m_ioc, m_storage_maintainer,
                      etcd),
          m_deduplicator(
              std::make_shared<local_deduplicator>(config, m_data_view)),
          m_server(config.server,
                   std::make_unique<deduplicator_handler>(*m_deduplicator),
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

    ~deduplicator() {
        LOG_DEBUG() << "terminating " << m_service_registry.get_service_name();
        m_ioc.stop();
    }

private:
    std::size_t m_service_id;
    boost::asio::io_context m_ioc;

    service_registry m_service_registry;

    attached_service<storage> m_attached_storage;
    service_maintainer<storage_interface> m_storage_maintainer;

    concrete_global_data_view m_data_view;
    std::shared_ptr<local_deduplicator> m_deduplicator;
    server m_server;
};
} // end namespace uh::cluster

#endif // CORE_DEDUPE_NODE_H
