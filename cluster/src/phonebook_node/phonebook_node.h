//
// Created by masi on 7/17/23.
//

#ifndef CORE_PHONEBOOK_NODE_H
#define CORE_PHONEBOOK_NODE_H

#include <functional>
#include <iostream>
#include "common/cluster_config.h"
#include "phonebook_node_handler.h"

namespace uh::cluster {

class phonebook_node {
public:

    phonebook_node (int id, cluster_map cmap):
            m_cluster_map (std::move (cmap)),
            m_id (id),
            m_job_name ("phonebook_" + std::to_string (id)),
            m_server (m_cluster_map.m_cluster_conf.phonebook_node_conf.server_conf,
                      std::make_unique <phonebook_node_handler>()),
            m_storage (m_cluster_map,
                       m_cluster_map.m_cluster_conf.phonebook_node_conf.data_node_connection_count,
                       m_cluster_map.m_cluster_conf.phonebook_node_conf.server_conf.threads)
    {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;
        m_server.run();
        while (!m_stop) {
            m_stop = true;
        }
    }

    const cluster_map m_cluster_map;
    const int m_id;
    const std::string m_job_name;
    server m_server;
    global_data m_storage;
    std::atomic <bool> m_stop = false;

};

} // end namespace uh::cluster

#endif //CORE_PHONEBOOK_NODE_H
