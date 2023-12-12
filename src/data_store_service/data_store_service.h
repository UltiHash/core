//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include "common/cluster_config.h"
#include "data_store.h"
#include "network/server.h"
#include "common/service_interface.h"
#include <atomic>
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <utility>
#include "data_store_service_handler.h"

namespace uh::cluster {
class data_store_service: public service_interface {
public:

    data_store_service(std::size_t id, std::string  service_key, const std::string& etcd_host) :
            m_id(id),
            m_service_key(std::move(service_key)),
            m_service_name(m_service_key + "/" + std::to_string(m_id)),
            m_etcd_client(etcd_host),
            m_server(make_data_node_config().server_conf, m_service_name,
                     std::make_unique<data_store_service_handler>(make_data_node_config(), id))
    {}

    void run() override {
        register_service();
        m_server.run();
    }

    void stop() override {
        m_server.stop();
        unregister_service();
    }

    ~data_store_service() override {
        LOG_DEBUG() << "terminating " << m_service_name;
    }

private:
    const std::size_t m_id;
    const std::string m_service_key;
    const std::string m_service_name;

    etcd::Client m_etcd_client;
    std::shared_ptr<etcd::KeepAlive> m_etcd_keepalive;

    server m_server;


    void register_service() {
        m_etcd_keepalive = m_etcd_client.leasekeepalive(etcd_default_ttl).get();
        std::string key = etcd_default_key_prefix + m_service_name;
        m_etcd_client.set(key, boost::asio::ip::host_name(), m_etcd_keepalive->Lease());
    }

    void unregister_service() {
        m_etcd_keepalive->Cancel();
    }

};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
