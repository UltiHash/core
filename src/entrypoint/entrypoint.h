//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>
#include <iostream>

#include "common/utils/cluster_config.h"
#include "common/utils/service_interface.h"
#include "common/registry/config_registry.h"
#include "common/registry/service_registry.h"
#include "common/network/server.h"
#include "common/network/client.h"
#include "rest_server.h"

namespace uh::cluster {
class entrypoint: public service_interface {
public:

    explicit entrypoint(const std::string& registry_url) :
            m_config_registry(uh::cluster::ENTRYPOINT_SERVICE, registry_url),
            m_service_registry(uh::cluster::ENTRYPOINT_SERVICE, m_config_registry.get_service_id() , registry_url),
            m_config(m_config_registry.get_entrypoint_config()),
            m_workers (std::make_shared <boost::asio::thread_pool> (m_config.worker_thread_count)),
            m_rest_server (m_config_registry.get_server_config(), m_dedupe_nodes, m_directory_nodes, m_workers)
    {
    }

    void run() override {
        m_service_registry.wait_for_dependency(uh::cluster::DEDUPLICATOR_SERVICE);
        m_service_registry.wait_for_dependency(uh::cluster::DIRECTORY_SERVICE);
        create_connections();
        m_registration = m_service_registry.register_service(m_rest_server.get_server_config());
        m_rest_server.run();
    }

    void stop() override {
        LOG_INFO() << "stopping " << m_service_registry.get_service_name();
        m_workers->join();
        m_workers->stop();
    }


private:
    config_registry m_config_registry;
    service_registry m_service_registry;
    entrypoint_config m_config;

    std::vector <std::shared_ptr <client>> m_dedupe_nodes;
    std::vector <std::shared_ptr <client>> m_directory_nodes;

    std::shared_ptr <boost::asio::thread_pool> m_workers;
    rest::rest_server m_rest_server;

    std::unique_ptr<service_registry::registration> m_registration;

    void create_connections() {
        for(const auto& instance : m_service_registry.get_service_instances(uh::cluster::DEDUPLICATOR_SERVICE)) {
            m_dedupe_nodes.emplace_back (std::make_shared <client> (m_rest_server.get_executor(), instance.host,
                                                                    instance.port,
                                                                    m_config.dedupe_node_connection_count));
        }

        for(const auto& instance : m_service_registry.get_service_instances(uh::cluster::DIRECTORY_SERVICE)) {
            m_directory_nodes.emplace_back(std::make_shared <client> (m_rest_server.get_executor(), instance.host,
                                                                      instance.port,
                                                                      m_config.directory_connection_count));
        }
    }

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
