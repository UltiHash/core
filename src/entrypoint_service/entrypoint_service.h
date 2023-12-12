//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>
#include <iostream>
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include "common/cluster_config.h"
#include "network/server.h"
#include "rest_server.h"
#include "network/client.h"
#include "common/service_interface.h"

namespace uh::cluster
{

class entrypoint_service: public service_interface {
public:

    entrypoint_service(std::size_t id, std::string  service_key, const std::string& etcd_host, const bool use_id_as_port_offset = false) :
            m_id(id),
            m_service_key(std::move(service_key)),
            m_service_name(m_service_key + "/" + std::to_string(m_id)),
            m_etcd_client(etcd_host),
            m_workers (std::make_shared <boost::asio::thread_pool> (make_entry_node_config().worker_thread_count)),
            m_rest_server (make_entry_node_config(), m_dedupe_nodes, m_directory_nodes, m_workers)
    {
        //TODO: wait for dependencies to be available before creating connections
        create_connections();
    }

    void run() override {
        register_service();
        m_rest_server.run();
    }

    void stop() override {
        LOG_INFO() << "stopping " << m_service_name;
        m_workers->join();
        m_workers->stop();
        unregister_service();
    }


private:

    const std::size_t m_id;
    const std::string m_service_key;
    const std::string m_service_name;

    etcd::Client m_etcd_client;
    std::shared_ptr<etcd::KeepAlive> m_etcd_keepalive;

    std::vector <std::shared_ptr <client>> m_dedupe_nodes;
    std::vector <std::shared_ptr <client>> m_directory_nodes;

    std::shared_ptr <boost::asio::thread_pool> m_workers;
    rest::rest_server m_rest_server;

    void register_service() {
        m_etcd_keepalive = m_etcd_client.leasekeepalive(etcd_default_ttl).get();
        std::string key = etcd_default_key_prefix + m_service_name;
        m_etcd_client.set(key, boost::asio::ip::host_name(), m_etcd_keepalive->Lease());
    }

    void unregister_service() {
        m_etcd_keepalive->Cancel();
    }


    void create_connections() {
        etcd::Response dd_instances = m_etcd_client.ls("/uh/dd").get();
        for (int i = 0; i < dd_instances.keys().size(); i++) {
            m_dedupe_nodes.emplace_back (std::make_shared <client> (m_rest_server.get_executor(), dd_instances.value(i).as_string(),
                                                                    make_dedupe_node_config().server_conf.port,
                                                                    make_entry_node_config().dedupe_node_connection_count));
        }

        etcd::Response dr_instances = m_etcd_client.ls("/uh/dr").get();
        for (int i = 0; i < dr_instances.keys().size(); i++) {
            m_directory_nodes.emplace_back(std::make_shared <client> (m_rest_server.get_executor(), dr_instances.value(i).as_string(),
                                                                      make_directory_node_config().server_conf.port,
                                                                      make_entry_node_config().directory_connection_count));
        }
    }

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
