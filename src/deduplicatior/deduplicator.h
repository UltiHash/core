#ifndef CORE_DEDUPE_NODE_H
#define CORE_DEDUPE_NODE_H

#include <functional>
#include <iostream>
#include <utility>
#include "common/utils/cluster_config.h"
#include "common/utils/log.h"
#include "common/global_data/global_data_view.h"
#include "deduplicator_handler.h"



namespace uh::cluster {
    class deduplicator: service_interface {
    public:

        deduplicator(std::size_t id, const bool use_id_as_port_offset = false) :
                m_id(id),
                m_service_name(abbreviation_by_role.at(uh::cluster::DEDUPLICATION_SERVICE) + "/" + std::to_string(m_id)),
                m_registry(m_service_name),
                m_dedupe_workers (std::make_shared <boost::asio::thread_pool> (make_dedupe_node_config().worker_thread_count)),
                m_storage (m_registry),
                m_server (make_dedupe_node_config().server_conf, m_service_name,
                          std::make_unique <deduplicator_handler>(make_dedupe_node_config(), m_storage, m_dedupe_workers)),
                m_use_id_as_port_offset (use_id_as_port_offset)
        {}

        void run() override {
            //TODO: wait for dependencies to be available before creating connections
            m_storage.create_data_node_connections(m_server.get_executor(), m_use_id_as_port_offset);
            m_registry.register_service();
            m_server.run();
        }

        void stop() override {
            m_server.stop();
            m_dedupe_workers->join();
            m_dedupe_workers->stop();
            m_registry.unregister_service();
        }

        ~deduplicator() override {
            LOG_DEBUG() << "terminating " << m_service_name;
        }

        global_data_view& get_global_data_view() {
            return m_storage;
        }

        server& get_server() {
            return m_server;
        }

    private:
        const std::size_t m_id;
        const std::string m_service_name;
        service_registry m_registry;

        std::shared_ptr <boost::asio::thread_pool> m_dedupe_workers;
        global_data_view m_storage;
        server m_server;
        const bool m_use_id_as_port_offset;

    };
} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_H
