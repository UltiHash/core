#ifndef CORE_DEDUPE_NODE_H
#define CORE_DEDUPE_NODE_H

#include <functional>
#include <iostream>
#include <utility>
#include "common/utils/cluster_config.h"
#include "common/utils/log.h"
#include "common/global_data/global_data_view.h"
#include "deduplicator_handler.h"
#include "common/utils/service_interface.h"
#include "common/registry/service_registry.h"
#include "common/network/server.h"

namespace uh::cluster {
    class deduplicator : public service_interface {
    public:

        explicit deduplicator(std::size_t id, const std::string& registry_url) :
                m_config_registry(DEDUPLICATOR_SERVICE, id, registry_url),
                m_service_registry(DEDUPLICATOR_SERVICE, id, registry_url),
                m_config(m_config_registry.get_deduplicator_config()),
                m_storage_services(m_ioc, m_config_registry, m_config_registry.get_global_data_view_config().storage_service_connection_count, registry_url),
                m_dedupe_workers (std::make_shared <boost::asio::thread_pool> (m_config.worker_thread_count)),
                m_ioc (boost::asio::io_context (m_config_registry.get_server_config().threads)),
                m_storage (m_config_registry.get_global_data_view_config(), m_ioc, m_storage_services),
                m_server (m_config_registry.get_server_config(), m_config_registry.get_service_name(),
                          std::make_unique <deduplicator_handler>(m_config, m_storage, m_dedupe_workers),
                          m_ioc)
        {}

        void run() override {
            m_service_registry.wait_for_dependency(uh::cluster::STORAGE_SERVICE);
            m_storage.create_storage_service_connections(m_service_registry);
            m_registration = m_service_registry.register_service(m_server.get_server_config());
            m_server.run();
        }

        void stop() override {
            m_server.stop();
            m_dedupe_workers->join();
            m_dedupe_workers->stop();
        }

        ~deduplicator() override {
            LOG_DEBUG() << "terminating " << m_service_registry.get_service_name();
            m_ioc.stop();
        }

    private:
        config_registry m_config_registry;
        service_registry m_service_registry;

        deduplicator_config m_config;

        services<STORAGE_SERVICE> m_storage_services;

        std::shared_ptr <boost::asio::thread_pool> m_dedupe_workers;
        boost::asio::io_context m_ioc;

        global_data_view m_storage;
        server m_server;
        std::unique_ptr<service_registry::registration> m_registration;

    };
} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_H
