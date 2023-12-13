//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include <atomic>
#include <utility>

#include "common/cluster_config.h"
#include "common/service_interface.h"
#include "common/service_registry.h"
#include "network/server.h"
#include "data_store.h"
#include "data_store_service_handler.h"

namespace uh::cluster {
class data_store_service: public service_interface {
public:

    explicit data_store_service(std::size_t id) :
            m_id(id),
            m_service_name(abbreviation_by_role.at(uh::cluster::DATASTORE_SERVICE) + "/" + std::to_string(m_id)),
            m_registry(m_service_name),
            m_server(make_data_node_config().server_conf, m_service_name,
                     std::make_unique<data_store_service_handler>(make_data_node_config(), id))
    {}

    void run() override {
        m_registry.register_service();
        m_server.run();
    }

    void stop() override {
        m_server.stop();
        m_registry.unregister_service();
    }

    ~data_store_service() override {
        LOG_DEBUG() << "terminating " << m_service_name;
    }

private:
    const std::size_t m_id;
    const std::string m_service_name;
    service_registry m_registry;

    server m_server;
};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
