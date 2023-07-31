//
// Created by masi on 7/28/23.
//
#include "common.h"
#include <mpi.h>
#include <stdexcept>
#include <string>
#include <cstring>

namespace uh::cluster {

    void handle_failure (const std::string& job_name, const int target, const std::exception &e) {
        const auto size = static_cast <int> (strlen (e.what()));
        const auto rc = MPI_Send(e.what(), size, MPI_CHAR, target, message_types::FAILURE, MPI_COMM_WORLD);
        if (rc != MPI_SUCCESS) [[unlikely]] {
            throw std::runtime_error(job_name + " could not inform the failure");
        }
    }

} // end namespace uh::cluster
