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

        explicit deduplicator(std::size_t id, const bool use_id_as_port_offset = false) :
                m_id(id),
                m_service_name(get_service_string(uh::cluster::DEDUPLICATOR_SERVICE) + "/" + std::to_string(m_id)),
                m_registry(m_service_name),
                m_dedupe_workers (std::make_shared <boost::asio::thread_pool> (make_deduplicator_config().worker_thread_count)),
                m_storage (m_registry),
                m_server (make_deduplicator_config().server_conf, m_service_name,
                          std::make_unique <deduplicator_handler>(make_deduplicator_config(), m_storage, m_dedupe_workers)),
                m_use_id_as_port_offset (use_id_as_port_offset)
        {
        }

        void run() override {
            m_registry.wait_for_dependency(uh::cluster::STORAGE_SERVICE);
            m_storage.create_data_node_connections(m_server.get_executor(), m_use_id_as_port_offset);
            m_registry.register_service();
            m_server.run();
        }

        void stop() override {
            m_server.stop();
            m_dedupe_workers->join();
            m_dedupe_workers->stop();
        }

        ~deduplicator() override {
            LOG_DEBUG() << "terminating " << m_service_name;
        }

        global_data_view& get_global_data_view() {
            return m_storage;
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
