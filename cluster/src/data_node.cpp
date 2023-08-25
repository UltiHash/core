//
// Created by masi on 7/27/23.
//

#include "data_node.h"

namespace uh::cluster {

data_node::data_node(int id, data_node_config conf, cluster_map&& cmap):
        m_cluster_map (std::move (cmap)),
        m_job_name ("data_node_" + std::to_string (id)),
        m_server (conf.server_conf),
        m_data_store (std::move (conf), id)
        {

        }

void data_node::run() {
    std::cout << "hello from " << m_job_name << std::endl;

    m_server.run();

}

} // end namespace uh::cluster
