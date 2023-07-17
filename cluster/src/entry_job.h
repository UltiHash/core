//
// Created by masi on 7/17/23.
//

#ifndef CORE_ENTRY_JOB_H
#define CORE_ENTRY_JOB_H

#include <functional>
#include <iostream>
#include "cluster_config.h"

namespace uh::cluster {

class entry_job {
public:

    entry_job (int id, std::shared_ptr <const uh::cluster::cluster_ranks> cluster_plan):
            m_cluster_plan (std::move (cluster_plan)),
            m_id (id),
            m_job_name ("entry_" + std::to_string (id)) {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;
    }

    const std::shared_ptr <const cluster_ranks> m_cluster_plan;
    const int m_id;
    const std::string m_job_name;
};

} // end namespace uh::cluster

#endif //CORE_ENTRY_JOB_H
