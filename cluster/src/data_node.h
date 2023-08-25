//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include "cluster_config.h"
#include "data_store.h"
#include "server.h"
#include "cluster_map.h"
#include <atomic>

namespace uh::cluster {
class data_node {
public:

    data_node (int id, cluster_map&& cmap);

    void run();

private:

    const std::string m_job_name;
    cluster_map m_cluster_map;
    server m_server;
    uh::cluster::data_store m_data_store;
    std::atomic <bool> m_stop = false;
};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
