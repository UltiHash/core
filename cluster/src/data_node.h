//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include "cluster_config.h"

namespace uh::cluster {
class data_node {
public:

    data_node (int id, const uh::cluster::cluster_ranks& cluster_plan):
            m_cluster_plan (cluster_plan),
            m_id (id),
            m_job_name ("data_node_" + std::to_string (id)) {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;
    }

    const std::reference_wrapper <const cluster_ranks> m_cluster_plan;
    const int m_id;
    const std::string m_job_name;

};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
