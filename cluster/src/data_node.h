//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include "cluster_config.h"
#include "data_store.h"
#include "storage/growing_managed_storage.h"

namespace uh::cluster {
class data_node {
public:

    data_node (int id, std::shared_ptr <const cluster_ranks> cluster_plan, data_node_config conf):
            m_cluster_plan (std::move (cluster_plan)),
            m_id (id),
            m_job_name ("data_node_" + std::to_string (id)),
            m_conf (std::move(conf)),
            m_data_store (conf.data_store_conf) {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;
    }

    const std::shared_ptr <const cluster_ranks> m_cluster_plan;
    const int m_id;
    const std::string m_job_name;
    data_node_config m_conf;
    uh::cluster::storage::growing_managed_storage m_data_store;

};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
