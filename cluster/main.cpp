//
// Created by masi on 7/17/23.
//

#include <mpi.h>
#include <system_error>
#include <cerrno>
#include "src/cluster_config.h"
#include <memory>
#include "src/data_node.h"
#include "src/dedupe_job.h"
#include "src/phonebook_job.h"
#include "src/entry_job.h"

uh::cluster::cluster_skeleton make_cluster_skeleton () {
    return {
        .data_node_jobs_count = 4,
        .dedupe_jobs_count = 2,
        .phonebook_jobs_count = 1,
        .entry_jobs_count= 1,
    };
}

uh::cluster::data_store_config make_data_store_config () {
    return {
        .directory = "root/dn",
        .hole_log = "root/dn/log",
        .min_file_size = 1ul * 1024ul * 1024ul,
        .max_file_size = 4ul * 1024ul * 1024ul,
        .max_data_store_size = 32ul * 1024ul * 1024ul,
    };
}

uh::cluster::global_data_config make_global_data_config () {
    return {
            .max_data_store_size =  64ul * 1024ul * 1024ul,
    };
}

uh::cluster::dedupe_config make_dedupe_node_config () {
    return {
        .min_fragment_size = 1024,
        .max_fragment_size = 4 * 1024,
        .storage_conf = make_global_data_config (),
        .set_conf = {
                .set_minimum_free_space = 1ul * 1024ul * 1024ul,
                .max_empty_hole_size = 1ul * 1024ul * 1024ul,
                .key_store_config = {
                        .file  = "root/set",
                        .init_size = 1ul * 1024ul * 1024ul,
                }
        },
    };
}

void execute_role (const int rank, const uh::cluster::cluster_skeleton& cluster_conf) {

    uh::cluster::cluster_ranks cluster_plan (cluster_conf);

    if (rank <= cluster_plan.data_node_ranks.back()) {
        const auto id = rank - cluster_plan.data_node_ranks.front();

        for (int i = 0; i < cluster_plan.dedupe_ranks.size(); i++) {
            MPI_Comm_split(MPI_COMM_WORLD, uh::cluster::communities::DEDUPE_DATA_NODES, id + 1,
                           &uh::cluster::dedupe_data_comm);
        }
        for (int i = 0; i < cluster_plan.phonebook_ranks.size(); i++) {
            MPI_Comm_split(MPI_COMM_WORLD, uh::cluster::communities::PHONEBOOK_DATA_NODES, id + 1,
                           &uh::cluster::phonebook_data_comm);
        }

        uh::cluster::data_node dn (make_data_store_config(), id);
        dn.run();
    }
    else if (rank <= cluster_plan.dedupe_ranks.back()) {
        const auto id = rank - cluster_plan.dedupe_ranks.front();
        MPI_Comm_split(MPI_COMM_WORLD, uh::cluster::communities::DEDUPE_DATA_NODES, 0, &uh::cluster::dedupe_data_comm);
        for (int i = 0; i < cluster_plan.entry_ranks.size(); i++) {
            MPI_Comm_split(MPI_COMM_WORLD, uh::cluster::communities::ENTRY_DEDUPE_NODES, id + 1,
                           &uh::cluster::entry_dedupe_comm);
        }
        uh::cluster::dedupe_job dd (rank, make_dedupe_node_config (), std::move (cluster_plan));
        dd.run();
    }
    else if (rank <= cluster_plan.phonebook_ranks.back()) {
        MPI_Comm_split(MPI_COMM_WORLD, uh::cluster::communities::PHONEBOOK_DATA_NODES, 0, &uh::cluster::phonebook_data_comm);
        uh::cluster::phonebook_job pb (rank, make_global_data_config(), std::move (cluster_plan));
        pb.run();
    }
    else if (rank <= cluster_plan.entry_ranks.back()) {
        for (int i = 0; i < cluster_plan.dedupe_ranks.size(); i++) {
            MPI_Comm_split(MPI_COMM_WORLD, uh::cluster::communities::ENTRY_DEDUPE_NODES, 0,
                           &uh::cluster::entry_dedupe_comm);
        }
        uh::cluster::entry_job en (rank, std::move (cluster_plan));
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

    const auto cluster_conf = make_cluster_skeleton();
    if (total_jobs != cluster_conf.entry_jobs_count  +
                    cluster_conf.phonebook_jobs_count  +
                    cluster_conf.dedupe_jobs_count +
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