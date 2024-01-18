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
#include "common/utils/services.h"
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
            m_dedupe_nodes(DEDUPLICATOR_SERVICE, m_registry, m_ioc,
            make_deduplicator_config().server_conf.port, make_entrypoint_config().dedupe_node_connection_count),
            m_directory_nodes(DIRECTORY_SERVICE, m_registry, m_ioc,
                                                   make_directory_config().server_conf.port, make_entrypoint_config().directory_connection_count),
            m_ioc (boost::asio::io_context(make_entrypoint_config().rest_server_conf.threads)),
            m_workers (std::make_shared <boost::asio::thread_pool> (make_entrypoint_config().worker_thread_count)),
            m_rest_server (make_entrypoint_config(), m_dedupe_nodes, m_directory_nodes, m_workers, m_ioc)
    {
    }

    void run() override {
        m_registry.wait_for_dependency(uh::cluster::DEDUPLICATOR_SERVICE);
        m_registry.wait_for_dependency(uh::cluster::DIRECTORY_SERVICE);

        create_connections(m_dedupe_nodes);
        create_connections(m_directory_nodes);

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

    services m_dedupe_nodes;
    services m_directory_nodes;

    boost::asio::io_context m_ioc;
    std::shared_ptr <boost::asio::thread_pool> m_workers;
    rest::rest_server m_rest_server;

    std::unique_ptr<service_registry::registration> m_registration;

    void create_connections (services& clients) {

        std::vector<std::pair<std::size_t, std::string>> ds_instances = m_registry.get_service_instances(clients.get_role());

        for(const auto& instance : ds_instances) {
            clients.add_service({.role = clients.get_role(), .id = instance.first , .value = instance.second});
        }
    }

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
