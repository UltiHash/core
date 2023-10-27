//
// Created by masi on 7/17/23.
//

#ifndef CORE_DIRECTORY_NODE_H
#define CORE_DIRECTORY_NODE_H

#include <functional>
#include <iostream>
#include "common/cluster_config.h"
#include "directory_node_handler.h"

namespace uh::cluster {

class directory_node: public node_interface {
public:

    directory_node (int id, cluster_map cmap):
            m_cluster_map (std::move (cmap)),
            m_id (id),
            m_job_name ("directory_" + std::to_string (id)),
            m_storage (m_cluster_map),
            m_server (m_cluster_map.m_cluster_conf.directory_node_conf.server_conf,
                      std::make_unique <directory_handler>(m_cluster_map.m_cluster_conf.directory_node_conf.directory_conf, m_storage))

    {
    }

    void run() override {
        std::cout << "hello from " << m_job_name << std::endl;
        m_storage.create_data_node_connections(m_server.get_executor(), m_cluster_map.m_cluster_conf.directory_node_conf.data_node_connection_count, false);
        if (m_id == 0) {
            //m_storage.recover();
        }
        m_server.run();
    }

    void stop () override {
        m_server.stop();
    }

    const cluster_map m_cluster_map;
    const int m_id;
    const std::string m_job_name;
    global_data_view m_storage;
    server m_server;


};

} // end namespace uh::cluster

#endif //CORE_DIRECTORY_NODE_H
