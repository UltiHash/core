//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>
#include <iostream>

#include "common/utils/cluster_config.h"
#include "common/utils/service_interface.h"
#include "common/utils/service_registry.h"
#include "common/utils/service_maintainer.h"
#include "common/network/server.h"
#include "common/network/client.h"
#include "rest_server.h"

namespace uh::cluster {
class entrypoint: public service_interface {
public:

    explicit entrypoint(std::size_t id, const std::string& registry_url) :
            m_id(id),
            m_service_name (get_service_string(uh::cluster::STORAGE_SERVICE) + "/" + std::to_string(m_id)),
            m_registry (m_service_name, registry_url),
            m_workers (std::make_shared <boost::asio::thread_pool> (make_entrypoint_config().worker_thread_count)),
            m_rest_server (make_entrypoint_config(), m_dedupe_nodes, m_directory_nodes, m_workers)
    {
        // TODO: refactor the parameter list of the constructor further further
        m_dedupe_nodes = std::make_unique<services>(DEDUPLICATOR_SERVICE, m_registry, m_rest_server.get_executor(),
                                                    make_deduplicator_config().server_conf.port, make_entrypoint_config().dedupe_node_connection_count);
        m_directory_nodes = std::make_unique<services>(DIRECTORY_SERVICE, m_registry, m_rest_server.get_executor(),
                                                       make_directory_config().server_conf.port, make_entrypoint_config().directory_connection_count);
    }

    void run() override {
        m_registry.wait_for_dependency(uh::cluster::DEDUPLICATOR_SERVICE);
        m_registry.wait_for_dependency(uh::cluster::DIRECTORY_SERVICE);

        m_registration = m_registry.register_service();
        m_rest_server.run();
    }

    void stop() override {
        LOG_INFO() << "stopping " << m_service_name;
        m_workers->join();
        m_workers->stop();
    }

private:
    const std::size_t m_id;
    const std::string m_service_name;
    service_registry m_registry;

    std::unique_ptr<services> m_dedupe_nodes;
    std::unique_ptr<services> m_directory_nodes;

    std::shared_ptr <boost::asio::thread_pool> m_workers;
    rest::rest_server m_rest_server;

    std::unique_ptr<service_registry::registration> m_registration;

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
