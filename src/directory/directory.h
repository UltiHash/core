#ifndef CORE_DIRECTORY_NODE_H
#define CORE_DIRECTORY_NODE_H

#include <functional>
#include <iostream>
#include <common/utils/log.h>
#include "common/utils/cluster_config.h"
#include "directory_handler.h"

namespace uh::cluster {

class directory: public service_interface {
public:

    explicit directory(const std::string& registry_url, const std::filesystem::path& working_dir) :
            m_config_registry(uh::cluster::DIRECTORY_SERVICE, registry_url, working_dir),
            m_ioc (boost::asio::io_context (m_config_registry.get_server_config().threads)),
            m_service_registry(uh::cluster::DIRECTORY_SERVICE, m_config_registry.get_service_id(), registry_url),
            m_storage_services(m_ioc, m_config_registry, m_config_registry.get_global_data_view_config().storage_service_connection_count, registry_url),
            m_config(m_config_registry.get_directory_config()),
            m_directory_workers (std::make_shared <boost::asio::thread_pool> (m_config.worker_thread_count)),
            m_storage (m_config_registry.get_global_data_view_config(), m_ioc, m_storage_services),
            m_server (m_config_registry.get_server_config(), m_config_registry.get_service_name(),
                      std::make_unique <directory_handler>(m_config, m_storage, m_directory_workers),
                      m_ioc) {
    }

    void run() override {
        m_registration = m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() override {
        m_server.stop();
        m_directory_workers->join();
        m_directory_workers->stop();
    }

private:
    config_registry m_config_registry;
    boost::asio::io_context m_ioc;
    service_registry m_service_registry;

    services<STORAGE_SERVICE> m_storage_services;

    directory_config m_config;
    std::shared_ptr <boost::asio::thread_pool> m_directory_workers;
    global_data_view m_storage;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};

} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_H
