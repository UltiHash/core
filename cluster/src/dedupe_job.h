//
// Created by masi on 7/17/23.
//

#ifndef CORE_DEDUPE_JOB_H
#define CORE_DEDUPE_JOB_H

#include <functional>
#include <iostream>
#include "cluster_config.h"
#include "global_data.h"

namespace uh::cluster {
class dedupe_job {
public:

    dedupe_job (int id, dedupe_config conf, cluster_ranks cluster_plan):
            m_cluster_plan (std::move (cluster_plan)),
            m_id (id),
            m_job_name ("dedupe_node_" + std::to_string (id)),
            m_storage (conf.storage_conf, cluster_plan.data_node_ranks) {

    }

    void run() {
        std::cout << "hello from " << m_job_name << std::endl;

        MPI_Status status;
        int message_size;

        while (!m_stop) {

            MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            try {
                switch (status.MPI_TAG) {
                    case message_types::DEDUPE:
                        MPI_Get_count (&status, MPI_CHAR, &message_size);
                        handle_dedupe (status.MPI_SOURCE, message_size);
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

    void handle_dedupe (int source, int data_size) {
        MPI_Status status;
        const auto buf = std::make_unique_for_overwrite<char []> (data_size);
        auto rc = MPI_Recv(buf.get(), data_size, MPI_CHAR, source, message_types::DEDUPE, MPI_COMM_WORLD, &status);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not receive the data for deduplication");
        }

        auto res = deduplicate ({buf.get(), static_cast <size_t> (data_size)});
        res.second.emplace_back(0, res.first); // last element contains the effective size
        const auto size = static_cast <int> (res.second.size() * sizeof (wide_span));
        rc = MPI_Send(res.second.data(), size, MPI_CHAR, source, message_types::WRITE_RESP, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error("Could not send the address of the written data");
        }
    }

    std::pair <std::size_t, address> deduplicate (std::string_view data) {
        return {};
    }

    const cluster_ranks m_cluster_plan;
    const int m_id;
    const std::string m_job_name;
    global_data m_storage;
    std::atomic <bool> m_stop = false;

};
} // end namespace uh::cluster

#endif //CORE_DEDUPE_JOB_H
