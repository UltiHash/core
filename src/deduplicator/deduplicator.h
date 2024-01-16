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
#include "common/utils/service_registry.h"
#include "common/network/server.h"

namespace uh::cluster {
    class deduplicator : public service_interface {
    public:

        explicit deduplicator(std::size_t id, const std::string& registry_url) :
                m_registry(uh::cluster::DEDUPLICATOR_SERVICE, id, registry_url),
                m_dedupe_workers (std::make_shared <boost::asio::thread_pool> (make_deduplicator_config().worker_thread_count)),
                m_storage (m_registry.get_global_data_view_config()),
                m_server (m_registry.get_server_config(), m_registry.get_service_name(),
                          std::make_unique <deduplicator_handler>(m_registry, m_storage, m_dedupe_workers)) {
        }

        void run() override {
            m_registry.wait_for_dependency(uh::cluster::STORAGE_SERVICE);
            m_storage.create_data_node_connections(m_server.get_executor(), m_registry.get_service_instances(uh::cluster::STORAGE_SERVICE));
            m_registration = m_registry.register_service();
            m_server.run();
        }

        void stop() override {
            m_server.stop();
            m_dedupe_workers->join();
            m_dedupe_workers->stop();
        }

        ~deduplicator() override {
            LOG_DEBUG() << "terminating " << m_registry.get_service_name();
        }

        global_data_view& get_global_data_view() {
            return m_storage;
        }

    private:
        service_registry m_registry;
        std::shared_ptr <boost::asio::thread_pool> m_dedupe_workers;
        global_data_view m_storage;
        server m_server;

        std::unique_ptr<service_registry::registration> m_registration;
    };
} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_H
