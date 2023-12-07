//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_NODE_H
#define CORE_ENTRY_NODE_H

#include <functional>
#include <iostream>
#include "common/cluster_config.h"
#include "network/cluster_map.h"
#include "network/server.h"
#include "entry_node/rest_server.h"
#include "network/client.h"

namespace uh::cluster
{

class entry_node {
public:

    entry_node (int id, cluster_map cmap);

    void run();

    void stop();


private:

    void create_connections ();

    std::vector <std::shared_ptr <client>> m_dedupe_nodes;
    std::vector <std::shared_ptr <client>> m_directory_nodes;

    const cluster_map m_cluster_map;
    const int m_id;
    const std::string m_job_name;
    std::shared_ptr <boost::asio::thread_pool> m_workers;
    rest::rest_server m_rest_server;

};

} // end namespace uh::cluster

#endif //CORE_ENTRY_NODE_H
