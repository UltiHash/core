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
            m_service_name(get_service_string(uh::cluster::STORAGE_SERVICE) + "/" + std::to_string(m_id)),
            m_registry(m_service_name, registry_url),
            m_dedupe_nodes(DEDUPLICATOR_SERVICE, m_registry, m_rest_server),
            m_directory_nodes(DIRECTORY_SERVICE, m_registry, m_rest_server),
            m_workers (std::make_shared <boost::asio::thread_pool> (make_entrypoint_config().worker_thread_count)),
            m_rest_server (make_entrypoint_config(), m_dedupe_nodes, m_directory_nodes, m_workers)
    {
    }

    void run() override {
        m_registry.wait_for_dependency(uh::cluster::DEDUPLICATOR_SERVICE);
        m_registry.wait_for_dependency(uh::cluster::DIRECTORY_SERVICE);
        // remove connections
//        create_connections();
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

    service_maintainer m_dedupe_nodes;
    service_maintainer m_directory_nodes;

    std::shared_ptr <boost::asio::thread_pool> m_workers;
    rest::rest_server m_rest_server;

    std::unique_ptr<service_registry::registration> m_registration;

//    void create_connections() {
//        for(const auto& instance : m_registry.get_service_instances(uh::cluster::DEDUPLICATOR_SERVICE)) {
//            m_dedupe_nodes.add_node_client(instance.first, instance.second);
//        }
//
//        for(const auto& instance : m_registry.get_service_instances(uh::cluster::DIRECTORY_SERVICE)) {
//            m_directory_nodes.add_node_client(instance.first, instance.second);
//        }
//    }

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
