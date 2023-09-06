//
// Created by masi on 7/27/23.
//

#include "data_node.h"
#include "data_node_handler.h"

namespace uh::cluster {

data_node::data_node(int id, cluster_map&& cmap):
        m_cluster_map (std::move (cmap)),
        m_job_name ("data_node_" + std::to_string (id)),
        m_server (m_cluster_map.m_cluster_conf.data_node_conf.server_conf,
                  std::make_unique<data_node_handler> (m_cluster_map.m_cluster_conf.data_node_conf, id))
        {
        }

void data_node::run() {
    std::cout << "hello from " << m_job_name << std::endl;

    m_server.run();

}

} // end namespace uh::cluster
