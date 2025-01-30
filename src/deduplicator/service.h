#pragma once

#include "common/etcd/registry/service_id.h"
#include "common/etcd/registry/service_registry.h"
#include "common/network/server.h"
#include "common/service_interfaces/deduplicator_interface.h"
#include "common/telemetry/log.h"
#include "config.h"
#include "handler.h"
#include <functional>
#include <storage/service.h>
#include <utility>

namespace uh::cluster::deduplicator {

class service {
public:
    service(const service_config& sc, const deduplicator_config& config)
        : m_ioc(boost::asio::io_context(config.server.threads)),
          m_etcd{sc.etcd_config},
          m_service_id(get_service_id(m_etcd,
                                      get_service_string(DEDUPLICATOR_SERVICE),
                                      sc.working_dir)),
          m_service_registry(DEDUPLICATOR_SERVICE, m_service_id, m_etcd),
          m_storage(std::make_unique<global_data_view>(config.storage_interface,
                                                       m_ioc, m_etcd)),
          m_deduplicator(
              std::make_unique<local_deduplicator>(m_ioc, config, *m_storage)),
          m_server(config.server, std::make_unique<handler>(*m_deduplicator),
                   m_ioc) {}

    void run() {
        m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() { m_server.stop(); }

    size_t id() const noexcept { return m_service_id; }

private:
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    std::size_t m_service_id;

    service_registry m_service_registry;
    std::unique_ptr<storage_interface> m_storage;
    std::unique_ptr<deduplicator_interface> m_deduplicator;
    server m_server;
};

} // namespace uh::cluster::deduplicator
