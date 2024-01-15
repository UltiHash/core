//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include <atomic>
#include <utility>

#include "common/utils/cluster_config.h"
#include "common/utils/service_interface.h"
#include "common/utils/service_registry.h"
#include "common/network/server.h"
#include "data_store.h"
#include "storage_handler.h"

namespace uh::cluster {
class storage: public service_interface {
public:

    explicit storage(std::size_t id, const std::string& registry_url) :
            m_registry(uh::cluster::STORAGE_SERVICE, id, registry_url),
            m_server(m_registry.get_server_config(), m_registry.get_service_name(), std::make_unique<storage_handler>(m_registry))
    {}

    void run() override {
        m_registration = m_registry.register_service();
        m_server.run();
    }

    void stop() override {
        m_server.stop();
    }

    ~storage() override {
        LOG_DEBUG() << "terminating " << m_registry.get_service_name();
    }

private:
    service_registry m_registry;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
