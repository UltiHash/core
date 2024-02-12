#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>
#include <iostream>

#include "common/utils/cluster_config.h"
#include "common/utils/service_interface.h"
#include "common/registry/services.h"
#include "common/registry/config_registry.h"
#include "common/registry/service_registry.h"
#include "common/network/server.h"
#include "common/network/client.h"
#include "entrypoint_handler.h"

namespace uh::cluster {
class entrypoint: public service_interface {
public:

    explicit entrypoint(const std::string& registry_url, const std::filesystem::path& working_dir) :
            m_config_registry(uh::cluster::ENTRYPOINT_SERVICE, registry_url, working_dir),
            m_ioc (boost::asio::io_context(m_config_registry.get_server_config().threads)),
            m_service_registry(uh::cluster::ENTRYPOINT_SERVICE, m_config_registry.get_service_id() , registry_url),
            m_config(m_config_registry.get_entrypoint_config()),
            m_dedupe_services(m_ioc, m_config_registry, m_config.dedupe_node_connection_count, registry_url),
            m_directory_services(m_ioc, m_config_registry, m_config.directory_connection_count, registry_url),
            m_workers (std::make_shared <boost::asio::thread_pool> (m_config.worker_thread_count)),
            m_server (m_config_registry.get_server_config(),
                      m_config_registry.get_service_name(),
                      std::make_unique <entry::entrypoint_handler> (m_ioc, m_dedupe_services, m_directory_services, m_workers),
                    m_ioc)
    {
    }

    void run() override {
        m_registration = m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() override {
        LOG_INFO() << "stopping " << m_service_registry.get_service_name();
        m_server.stop();
        m_workers->join();
        m_workers->stop();
    }

private:

    config_registry m_config_registry;
    boost::asio::io_context m_ioc;

    service_registry m_service_registry;

    entrypoint_config m_config;

    services<DEDUPLICATOR_SERVICE> m_dedupe_services;
    services<DIRECTORY_SERVICE> m_directory_services;

    std::shared_ptr <boost::asio::thread_pool> m_workers;
    server m_server;

    std::unique_ptr<service_registry::registration> m_registration;

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
