//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_JOB_H
#define CORE_ENTRY_JOB_H

#include <functional>
#include <iostream>
#include "cluster_config.h"
#include "cluster_map.h"
#include "server.h"
#include "rest_api/rest_server.h"

namespace uh::cluster
{

class entry_job {
public:

    entry_job (int id, entry_node_config config, cluster_map&& cmap);

    void run();

private:

    const cluster_map m_cluster_map;
    const int m_id;
    const std::string m_job_name;
    server m_internal_server;
    uh::rest::rest_server m_rest_server;
};

} // end namespace uh::cluster

#endif //CORE_ENTRY_JOB_H
