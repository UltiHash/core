//
// Created by masi on 7/17/23.
//

#ifndef CORE_DATA_NODE_H
#define CORE_DATA_NODE_H

#include <functional>
#include <iostream>
#include "common/cluster_config.h"
#include "data_store.h"
#include "network/server.h"
#include "network/cluster_map.h"
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
    std::atomic <bool> m_stop = false;
};
} // end namespace uh::cluster
#endif //CORE_DATA_NODE_H
