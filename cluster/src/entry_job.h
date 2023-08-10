//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_JOB_H
#define CORE_ENTRY_JOB_H

#include <functional>
#include <iostream>
#include "cluster_config.h"
#include "rest_node/src/server/server.h"

namespace uh::cluster
{

    class entry_job
    {
    private:
        std::unique_ptr<uh::rest::rest_server> m_rest_server;
    public:

        entry_job (int id, uh::cluster::cluster_ranks cluster_plan, uh::rest::rest_server_config&& rest_config);

        void run();
        
        const cluster_ranks m_cluster_plan;
        const int m_id;
        const std::string m_job_name;
    };

} // end namespace uh::cluster

#endif //CORE_ENTRY_JOB_H
