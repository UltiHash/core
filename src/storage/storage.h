#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <utility>

#include "common/network/server.h"
#include "common/registry/service_id.h"
#include "common/registry/service_registry.h"
#include "config.h"
#include "storage_handler.h"

namespace uh::cluster {

class storage {
public:
    explicit storage(const service_config& service, const storage_config& sc)
        : m_etcd_client(service.etcd_url),
          m_service_id(get_service_id(m_etcd_client,
                                      get_service_string(STORAGE_SERVICE),
                                      service.working_dir)),
          m_ioc(sc.server.threads),
          m_storage(std::make_shared<local_storage>(m_service_id, sc.data_store,
                                                    sc.m_data_store_roots)),
          m_service_registry(STORAGE_SERVICE, m_service_id, m_etcd_client),
          m_server(sc.server, std::make_unique<storage_handler>(*m_storage),
                   m_ioc) {}

    void run() {
        m_registration =
            m_service_registry.register_service(m_server.get_server_config());
        m_registration->monitor(STORAGE_FREE_SPACE, [this] {
            return std::to_string(m_storage->get_free_space());
        });

        m_registration->monitor(STORAGE_LOAD, [this] {
            return std::to_string(m_storage->catch_load());
        });

        m_server.run();
    }

    void stop() { m_server.stop(); }

    ~storage() {
        LOG_DEBUG() << "terminating " << m_service_registry.get_service_name();
    }

    std::shared_ptr<local_storage> get_local_interface() { return m_storage; }

private:
    etcd::SyncClient m_etcd_client;
    std::size_t m_service_id;
    boost::asio::io_context m_ioc;
    std::shared_ptr<local_storage> m_storage;
    service_registry m_service_registry;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};
} // end namespace uh::cluster
#endif // CORE_DATA_NODE_H
