//
// Created by masi on 7/17/23.
//

#ifndef CORE_CLUSTER_CONFIG_H
#define CORE_CLUSTER_CONFIG_H

#include <vector>
#include <numeric>
#include <boost/multiprecision/cpp_int.hpp>

namespace uh::cluster {

typedef boost::multiprecision::uint128_t uint128_t;

struct cluster_skeleton {
    int data_node_jobs_count;
    int dedupe_jobs_count;
    int redupe_jobs_count;
    int phonebook_jobs_count;
    int entry_jobs_count;
};

struct cluster_ranks {
    std::vector <int> data_node_ranks;
    std::vector <int> dedupe_ranks;
    std::vector <int> redupe_ranks;
    std::vector <int> phonebook_ranks;
    std::vector <int> entry_ranks;

    explicit cluster_ranks (const cluster_skeleton& cluster_conf):
        data_node_ranks (cluster_conf.data_node_jobs_count),
        dedupe_ranks (cluster_conf.dedupe_jobs_count),
        redupe_ranks (cluster_conf.redupe_jobs_count),
        phonebook_ranks (cluster_conf.phonebook_jobs_count),
        entry_ranks (cluster_conf.entry_jobs_count) {

        size_t rank_offset = 0;
        std::iota (std::begin(data_node_ranks), std::end(data_node_ranks), rank_offset);
        rank_offset += data_node_ranks.size();
        std::iota (std::begin(dedupe_ranks), std::end(dedupe_ranks), rank_offset);
        rank_offset += dedupe_ranks.size();
        std::iota (std::begin(redupe_ranks), std::end(redupe_ranks), rank_offset);
        rank_offset += redupe_ranks.size();
        std::iota (std::begin(phonebook_ranks), std::end(phonebook_ranks), rank_offset);
        rank_offset += phonebook_ranks.size();
        std::iota (std::begin(entry_ranks), std::end(entry_ranks), rank_offset);

    }
};

struct data_store_config {
    uint128_t max_size;
};

} // end namespace uh::cluster

#endif //CORE_CLUSTER_CONFIG_H
