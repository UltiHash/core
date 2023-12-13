//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>
#include <iostream>

#include "common/cluster_config.h"
#include "common/service_interface.h"
#include "common/service_registry.h"
#include "network/server.h"
#include "network/client.h"
#include "rest_server.h"

namespace uh::cluster {
class entrypoint: public service_interface {
public:

    explicit entrypoint(std::size_t id) :
            m_id(id),
            m_service_name(abbreviation_by_role.at(uh::cluster::DATASTORE_SERVICE) + "/" + std::to_string(m_id)),
            m_registry(m_service_name),
            m_workers (std::make_shared <boost::asio::thread_pool> (make_entry_node_config().worker_thread_count)),
            m_rest_server (make_entry_node_config(), m_dedupe_nodes, m_directory_nodes, m_workers)
    {
        //TODO: wait for dependencies to be available before creating connections
        create_connections();
    }

    void run() override {
        m_registry.register_service();
        m_rest_server.run();
    }

    void stop() override {
        LOG_INFO() << "stopping " << m_service_name;
        m_workers->join();
        m_workers->stop();
        m_registry.unregister_service();
    }


private:
    const std::size_t m_id;
    const std::string m_service_name;
    service_registry m_registry;

    std::vector <std::shared_ptr <client>> m_dedupe_nodes;
    std::vector <std::shared_ptr <client>> m_directory_nodes;

    std::shared_ptr <boost::asio::thread_pool> m_workers;
    rest::rest_server m_rest_server;


    void create_connections() {
        for(const auto& instance : m_registry.get_service_instances(uh::cluster::DEDUPLICATION_SERVICE)) {
            m_dedupe_nodes.emplace_back (std::make_shared <client> (m_rest_server.get_executor(), instance.second,
                                                                    make_dedupe_node_config().server_conf.port,
                                                                    make_entry_node_config().dedupe_node_connection_count));
        }

        for(const auto& instance : m_registry.get_service_instances(uh::cluster::DIRECTORY_SERVICE)) {
            m_directory_nodes.emplace_back(std::make_shared <client> (m_rest_server.get_executor(), instance.second,
                                                                      make_directory_node_config().server_conf.port,
                                                                      make_entry_node_config().directory_connection_count));
        }
    }

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
