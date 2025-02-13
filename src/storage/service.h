#pragma once

#include <functional>
#include <utility>

#include "common/etcd/registry/service_id.h"
#include "common/etcd/registry/service_registry.h"
#include "common/network/server.h"
#include "config.h"
#include "config/configuration.h"
#include "handler.h"

namespace uh::cluster::storage {

class service {
public:
    service(const service_config& service, const storage_config& sc)
        : m_ioc(sc.server.threads),
          m_etcd{service.etcd_config},
          m_service_id(get_service_id(m_etcd,
                                      get_service_string(STORAGE_SERVICE),
                                      service.working_dir)),
          m_storage(std::make_shared<local_storage>(m_service_id, sc.data_store,
                                                    sc.data_store_roots)),
          m_service_registry(STORAGE_SERVICE, m_service_id, m_etcd),
          m_server(sc.server, std::make_unique<handler>(*m_storage), m_ioc) {}

    void run() {
        m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() { m_server.stop(); }

    size_t id() const noexcept { return m_service_id; }

    std::shared_ptr<local_storage> get_local_interface() { return m_storage; }

private:
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    std::size_t m_service_id;
    std::shared_ptr<local_storage> m_storage;
    service_registry m_service_registry;
    server m_server;
};
} // namespace uh::cluster::storage
