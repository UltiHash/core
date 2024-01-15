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
#include "common/utils/service_registry.h"
#include "common/network/server.h"
#include "storage_handler.h"

namespace uh::cluster {
class storage: public service_interface {
public:

    explicit storage(std::size_t id, const std::string& registry_url) :
            m_id(id),
            m_service_name(get_service_string(uh::cluster::STORAGE_SERVICE) + "/" + std::to_string(m_id)),
            m_registry(m_service_name, registry_url),
            m_server(make_storage_config().server_conf, m_service_name,
                     std::make_unique<storage_handler>(make_storage_config(), id))
    {}

    void run() override {
        m_registration = m_registry.register_service();
        m_server.run();
    }

    void stop() override {
        m_server.stop();
    }

    ~storage() override {
        LOG_DEBUG() << "terminating " << m_service_name;
    }

private:
    const std::size_t m_id;
    const std::string m_service_name;
    service_registry m_registry;

    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
