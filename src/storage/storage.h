#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <utility>

#include "common/etcd/registry/service_id.h"
#include "common/etcd/registry/service_registry.h"
#include "common/network/server.h"
#include "config.h"
#include "storage_handler.h"

namespace uh::cluster {

class storage {
public:
    storage(etcd_manager& etcd, const service_config& service,
            const storage_config& sc)
        : m_service_id(get_service_id(etcd, get_service_string(STORAGE_SERVICE),
                                      service.working_dir)),
          m_ioc(sc.server.threads),
          m_storage(std::make_shared<local_storage>(m_service_id, sc.data_store,
                                                    sc.m_data_store_roots)),
          m_service_registry(STORAGE_SERVICE, m_service_id, etcd),
          m_server(sc.server, std::make_unique<storage_handler>(*m_storage),
                   m_ioc) {}

    void run() {
        m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() { m_server.stop(); }

    size_t id() const noexcept { return m_service_id; }

    std::shared_ptr<local_storage> get_local_interface() { return m_storage; }

private:
    std::size_t m_service_id;
    boost::asio::io_context m_ioc;
    std::shared_ptr<local_storage> m_storage;
    service_registry m_service_registry;
    server m_server;
};
} // end namespace uh::cluster
#endif // CORE_DATA_NODE_H
