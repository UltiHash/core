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

    phonebook_job (int id, global_data_config storage_conf, cluster_map cmap):
            m_cluster_map (std::move (cmap)),
            m_id (id),
            m_job_name ("phonebook_" + std::to_string (id)),
            m_storage (storage_conf)
    {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;

        MPI_Status status;
        int message_size;

        while (!m_stop) {

            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            try {
                switch (status.MPI_TAG) {
                    case message_types::READ_REQ:
                        MPI_Get_count (&status, MPI_CHAR, &message_size);
                        handle_read (status.MPI_SOURCE, message_size);
                        break;
                    default:
                        throw std::invalid_argument("Unknown tag");
                }
            }
            catch (std::exception& e) {
                handle_failure (m_job_name, status.MPI_SOURCE, e);
            }
        }
    }

    void handle_read(int source, int size) {

    }

    const cluster_map m_cluster_map;
    const int m_id;
    const std::string m_job_name;
    global_data m_storage;
    std::atomic <bool> m_stop = false;

};

} // end namespace uh::cluster

#endif //CORE_PHONEBOOK_JOB_H
