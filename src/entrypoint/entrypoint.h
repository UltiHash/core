//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>
#include <iostream>

#include "common/utils/cluster_config.h"
#include "common/utils/service_interface.h"
#include "common/utils/services.h"
#include "common/registry/config_registry.h"
#include "common/registry/service_registry.h"
#include "common/network/server.h"
#include "common/network/client.h"
#include "rest_server.h"

namespace uh::cluster {
class entrypoint: public service_interface {
public:

    explicit entrypoint(std::size_t id, const std::string& registry_url) :
            m_config_registry(uh::cluster::ENTRYPOINT_SERVICE, id , registry_url),
            m_service_registry(uh::cluster::ENTRYPOINT_SERVICE, id , registry_url),
            m_dedupe_nodes(DEDUPLICATOR_SERVICE, m_service_registry, m_ioc, make_entrypoint_config().dedupe_node_connection_count),
            m_directory_nodes(DIRECTORY_SERVICE, m_service_registry, m_ioc, make_entrypoint_config().directory_connection_count),
            m_ioc (boost::asio::io_context(m_config_registry.get_server_config().threads)),
            m_workers (std::make_shared <boost::asio::thread_pool> (make_entrypoint_config().worker_thread_count)),
            m_rest_server (m_config_registry.get_server_config(), m_dedupe_nodes, m_directory_nodes, m_workers, m_ioc)
    {
    }

    void run() override {
        m_service_registry.wait_for_dependency(uh::cluster::DEDUPLICATOR_SERVICE);
        m_service_registry.wait_for_dependency(uh::cluster::DIRECTORY_SERVICE);

        create_connections(m_dedupe_nodes);
        create_connections(m_directory_nodes);

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

    services m_dedupe_nodes;
    services m_directory_nodes;

    boost::asio::io_context m_ioc;
    std::shared_ptr <boost::asio::thread_pool> m_workers;
    rest::rest_server m_rest_server;

    std::unique_ptr<service_registry::registration> m_registration;

    void create_connections (services& clients) {

        std::vector<service_endpoint> instances = m_service_registry.get_service_instances(clients.get_role());

        for(const auto& instance : instances) {
            clients.add_service(instance);
        }

    }

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
