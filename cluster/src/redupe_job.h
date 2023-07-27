//
// Created by masi on 7/17/23.
//

#ifndef CORE_REDUPE_JOB_H
#define CORE_REDUPE_JOB_H

#include <functional>
#include <iostream>
#include "cluster_config.h"

namespace uh::cluster {
class redupe_job {
public:

    redupe_job (int id, uh::cluster::cluster_ranks cluster_plan):
            m_cluster_plan (std::move (cluster_plan)),
            m_id (id),
            m_job_name ("redupe_node_" + std::to_string (id)) {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;
    }

    const cluster_ranks m_cluster_plan;
    const int m_id;
    const std::string m_job_name;

};

} // end namespace uh::cluster

#endif //CORE_REDUPE_JOB_H
