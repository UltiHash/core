//
// Created by masi on 7/17/23.
//

#ifndef CORE_PHONEBOOK_JOB_H
#define CORE_PHONEBOOK_JOB_H

#include <functional>
#include <iostream>
#include "cluster_config.h"

namespace uh::cluster {

class phonebook_job {
public:

    phonebook_job (int id, phonebook_node_config config, cluster_map cmap):
            m_cluster_map (std::move (cmap)),
            m_id (id),
            m_job_name ("phonebook_" + std::to_string (id)),
            m_server (config.server_conf),
            m_storage (config.storage_conf, m_cluster_map)
    {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;

        while (!m_stop) {
            m_stop = true;
        }
    }

    const cluster_map m_cluster_map;
    const int m_id;
    const std::string m_job_name;
    server m_server;
    global_data m_storage;
    std::atomic <bool> m_stop = false;

};

} // end namespace uh::cluster

#endif //CORE_PHONEBOOK_JOB_H
