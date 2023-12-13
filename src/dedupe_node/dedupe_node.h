#ifndef CORE_DEDUPE_NODE_H
#define CORE_DEDUPE_NODE_H

#include <functional>
#include <iostream>
#include <utility>
#include "common/utils/cluster_config.h"
#include "common/global_data/global_data_view.h"
#include "dedupe_node_handler.h"
#include "common/utils/log.h"

namespace uh::cluster {
    class dedupe_node: public node_interface {
    public:

        dedupe_node (int id, cluster_map cmap, const bool use_id_as_port_offset = false):
                m_cluster_map (std::move (cmap)),
                m_id (id),
                m_job_name ("dedupe_node_" + std::to_string (m_id)),
                m_dedupe_workers (std::make_shared <boost::asio::thread_pool> (m_cluster_map.m_cluster_conf.dedupe_node_conf.worker_thread_count)),
                m_storage (m_cluster_map),
                m_server (m_cluster_map.m_cluster_conf.dedupe_node_conf.server_conf, m_job_name,
                          std::make_unique <dedupe_node_handler>(m_cluster_map.m_cluster_conf.dedupe_node_conf, m_storage, m_dedupe_workers)),
                m_use_id_as_port_offset (use_id_as_port_offset)
        {}

        void run() override {
            LOG_INFO() << "starting " << m_job_name;
            m_storage.create_data_node_connections(m_server.get_executor(), m_cluster_map.m_cluster_conf.dedupe_node_conf.data_node_connection_count, m_use_id_as_port_offset);
            m_server.run();
        }

        void stop() override {
            LOG_INFO() << "stopping " << m_job_name;
            m_server.stop();
            m_dedupe_workers->join();
            m_dedupe_workers->stop();
        }


        global_data_view& get_global_data_view() {
            return m_storage;
        }

        server& get_server() {
            return m_server;
        }


    private:

        const cluster_map m_cluster_map;
        const int m_id;
        const std::string m_job_name;
        std::shared_ptr <boost::asio::thread_pool> m_dedupe_workers;
        global_data_view m_storage;
        server m_server;
        const bool m_use_id_as_port_offset;

    };
} // end namespace uh::cluster

#endif //CORE_DEDUPE_NODE_H
