//
// Created by masi on 7/17/23.
//

#ifndef CORE_DIRECTORY_NODE_H
#define CORE_DIRECTORY_NODE_H

#include <functional>
#include <iostream>
#include "common/utils/log.h"
#include "common/utils/cluster_config.h"
#include "directory_node_handler.h"

namespace uh::cluster {

class directory_node: public node_interface {
public:

    directory_node (int id, cluster_map cmap, const bool use_id_as_port_offset = false):
            m_cluster_map (std::move (cmap)),
            m_id (id),
            m_job_name ("directory_" + std::to_string (id)),
            m_directory_workers (std::make_shared <boost::asio::thread_pool> (m_cluster_map.m_cluster_conf.directory_node_conf.worker_thread_count)),
            m_storage (m_cluster_map),
            m_server (m_cluster_map.m_cluster_conf.directory_node_conf.server_conf, m_job_name,
                      std::make_unique <directory_handler>(m_cluster_map.m_cluster_conf.directory_node_conf, m_storage, m_directory_workers)),
            m_use_id_as_port_offset (use_id_as_port_offset)
    {
    }

    void run() override {
        LOG_INFO() << "starting " << m_job_name;
        m_storage.create_data_node_connections(m_server.get_executor(), m_cluster_map.m_cluster_conf.directory_node_conf.data_node_connection_count, m_use_id_as_port_offset);
        m_server.run();
    }

    void stop () override {
        LOG_INFO() << "stopping " << m_job_name;
        m_server.stop();
        m_directory_workers->join();
        m_directory_workers->stop();
    }

    global_data_view& get_global_data_view() {
        return m_storage;
    }

    server& get_server() {
        return m_server;
    }

    const cluster_map m_cluster_map;
    const int m_id;
    const std::string m_job_name;
    std::shared_ptr <boost::asio::thread_pool> m_directory_workers;
    global_data_view m_storage;
    server m_server;
    const bool m_use_id_as_port_offset;

};

} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_H
