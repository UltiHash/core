//
// Created by masi on 7/17/23.
//

#include <mpi.h>
#include "src/cluster_config.h"

uh::cluster::cluster_config make_cluster_config () {
    return {
        .data_node_jobs = 4,
        .dedupe_jobs = 2,
        .redupe_jobs = 2,
        .phonebook_jobs = 1,
        .entry_jobs = 1,
    };
};

void execute_role (const int rank, const uh::cluster::cluster_config& cluster_conf) {
    if (auto ranks_offset = cluster_conf.data_node_jobs; rank < ranks_offset) {
        // data node
    }
    else if (ranks_offset += cluster_conf.dedupe_jobs; rank < ranks_offset) {
        // dedupe node
    }
    else if (ranks_offset += cluster_conf.redupe_jobs; rank < ranks_offset) {
        // redupe node
    }
    else if (ranks_offset += cluster_conf.phonebook_jobs; rank < ranks_offset) {
        // phonebook node
    }
    else if (ranks_offset += cluster_conf.entry_jobs; rank < ranks_offset) {
        // entry node
    }
}

int main (int argc, char* argv[]) {
    auto rc = MPI_Init (&argc, &argv);
    if (rc != MPI_SUCCESS) {

    }

    int total_jobs;
    rc = MPI_Comm_size(MPI_COMM_WORLD, &total_jobs);
    if (rc != MPI_SUCCESS) {

    }

    const auto cluster_conf = make_cluster_config();
    if (total_jobs != cluster_conf.entry_jobs +
                    cluster_conf.phonebook_jobs +
                    cluster_conf.redupe_jobs +
                    cluster_conf.dedupe_jobs +
                    cluster_conf.data_node_jobs) {

    }

    int rank;
    rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rc != MPI_SUCCESS) {

    }

    execute_role (rank, cluster_conf);

    rc = MPI_Finalize();
    if (rc != MPI_SUCCESS) {

    }
}