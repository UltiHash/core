//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include <atomic>
#include <utility>

#include "data_store.h"
#include "common/utils/cluster_config.h"
#include "common/utils/service_interface.h"
#include "common/registry/config_registry.h"
#include "common/registry/service_registry.h"
#include "common/network/server.h"
#include "storage_handler.h"

namespace uh::cluster {
class storage: public service_interface {
public:

    explicit storage(std::size_t id, const std::string& registry_url) :
            m_config_registry(uh::cluster::STORAGE_SERVICE, id, registry_url),
            m_service_registry(uh::cluster::STORAGE_SERVICE, id, registry_url),
            m_ioc (boost::asio::io_context (m_config_registry.get_server_config().threads)),
            m_server(m_config_registry.get_server_config(), m_config_registry.get_service_name(),
                     std::make_unique<storage_handler>(m_config_registry.get_storage_config(), id), m_ioc)
    {}

    void run() override {
        m_registration = m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() override {
        m_server.stop();
        m_ioc.stop();
    }

    ~storage() override {
        LOG_DEBUG() << "terminating " << m_service_registry.get_service_name();
    }

private:
    config_registry m_config_registry;
    service_registry m_service_registry;
    boost::asio::io_context m_ioc;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
