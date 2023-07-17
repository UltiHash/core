//
// Created by masi on 7/17/23.
//

#include <mpi.h>
#include <system_error>
#include <cerrno>
#include "src/cluster_config.h"
#include "src/data_node.h"
#include "src/dedupe_job.h"
#include "src/redupe_job.h"
#include "src/phonebook_job.h"
#include "src/entry_job.h"

uh::cluster::cluster_skeleton make_cluster_config () {
    return {
        .data_node_jobs_count = 4,
        .dedupe_jobs_count = 2,
        .redupe_jobs_count = 2,
        .phonebook_jobs_count = 1,
        .entry_jobs_count= 1,
    };
};

void execute_role (const int rank, const uh::cluster::cluster_skeleton& cluster_conf) {

    uh::cluster::cluster_ranks cluster_plan (cluster_conf);

    if (rank <= cluster_plan.data_node_ranks.back()) {
        uh::cluster::data_node dn (rank, cluster_plan);
        dn.run();
    }
    else if (rank <= cluster_plan.dedupe_ranks.back()) {
        uh::cluster::dedupe_job dd (rank, cluster_plan);
        dd.run();
    }
    else if (rank <= cluster_plan.redupe_ranks.back()) {
        uh::cluster::redupe_job rd (rank, cluster_plan);
        rd.run();
    }
    else if (rank <= cluster_plan.phonebook_ranks.back()) {
        uh::cluster::phonebook_job pb (rank, cluster_plan);
        pb.run();
    }
    else if (rank <= cluster_plan.entry_ranks.back()) {
        uh::cluster::entry_job en (rank, cluster_plan);
        en.run();
    }
}

int main (int argc, char* argv[]) {
    auto rc = MPI_Init (&argc, &argv);
    if (rc != MPI_SUCCESS) {
        throw std::runtime_error ("MPI operation failed");
    }

    int total_jobs;
    rc = MPI_Comm_size(MPI_COMM_WORLD, &total_jobs);
    if (rc != MPI_SUCCESS) {
        throw std::runtime_error ("MPI operation failed");
    }

    const auto cluster_conf = make_cluster_config();
    if (total_jobs != cluster_conf.entry_jobs_count  +
                    cluster_conf.phonebook_jobs_count  +
                    cluster_conf.redupe_jobs_count  +
                    cluster_conf.dedupe_jobs_count  +
                    cluster_conf.data_node_jobs_count) {
        throw std::logic_error ("The number of processes must match the cluster configuration");
    }

    int rank;
    rc = MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rc != MPI_SUCCESS) {
        throw std::runtime_error ("MPI operation failed");
    }

    execute_role (rank, cluster_conf);

    rc = MPI_Finalize();
    if (rc != MPI_SUCCESS) {
        throw std::runtime_error ("MPI operation failed");
    }
}