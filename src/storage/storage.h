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

    explicit storage(const std::string& registry_url, const std::filesystem::path& working_dir) :
            m_config_registry(uh::cluster::STORAGE_SERVICE, registry_url, working_dir),
            m_ioc (m_config_registry.get_server_config().threads),
            m_service_registry(uh::cluster::STORAGE_SERVICE, m_config_registry.get_service_id(), registry_url),
            m_server(m_config_registry.get_server_config(), m_service_registry.get_service_name(),
                     std::make_unique<storage_handler>(m_config_registry.get_storage_config(), m_config_registry.get_service_id()), m_ioc)
    {
    }

    void run() override {
        m_registration = m_service_registry.register_service(m_server.get_server_config());
        m_server.run();
    }

    void stop() override {
        m_server.stop();
    }

    ~storage() override {
        LOG_DEBUG() << "terminating " << m_service_registry.get_service_name();
    }

private:
    config_registry m_config_registry;
    boost::asio::io_context m_ioc;
    service_registry m_service_registry;
    server m_server;
    std::unique_ptr<service_registry::registration> m_registration;
};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
