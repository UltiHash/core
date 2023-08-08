//
// Created by masi on 7/17/23.
//

#ifndef CORE_CLUSTER_CONFIG_H
#define CORE_CLUSTER_CONFIG_H

#include <vector>
#include <numeric>
#include <filesystem>
#include "common.h"

namespace uh::cluster {

struct cluster_skeleton {
    int data_node_jobs_count;
    int dedupe_jobs_count;
    int phonebook_jobs_count;
    int entry_jobs_count;
};

struct cluster_ranks {
    std::vector <int> data_node_ranks;
    std::vector <int> dedupe_ranks;
    std::vector <int> phonebook_ranks;
    std::vector <int> entry_ranks;

    explicit cluster_ranks (const cluster_skeleton& cluster_conf):
        data_node_ranks (cluster_conf.data_node_jobs_count),
        dedupe_ranks (cluster_conf.dedupe_jobs_count),
        phonebook_ranks (cluster_conf.phonebook_jobs_count),
        entry_ranks (cluster_conf.entry_jobs_count) {

        size_t rank_offset = 0;
        std::iota (std::begin(data_node_ranks), std::end(data_node_ranks), rank_offset);
        rank_offset += data_node_ranks.size();
        std::iota (std::begin(dedupe_ranks), std::end(dedupe_ranks), rank_offset);
        rank_offset += dedupe_ranks.size();
        std::iota (std::begin(phonebook_ranks), std::end(phonebook_ranks), rank_offset);
        rank_offset += phonebook_ranks.size();
        std::iota (std::begin(entry_ranks), std::end(entry_ranks), rank_offset);

    }
};

struct data_store_config {
    std::filesystem::path directory;
    std::filesystem::path hole_log;
    size_t min_file_size;
    size_t max_file_size;
    uint128_t max_data_store_size;
};

struct global_data_config {
    uint128_t max_data_store_size;
};


struct growing_plain_storage_config {
    std::filesystem::path file;
    size_t init_size;
};

struct set_config {
    unsigned long set_minimum_free_space;
    unsigned long max_empty_hole_size;
    growing_plain_storage_config key_store_config;
};

struct dedupe_config {
    std::size_t min_fragment_size;
    std::size_t max_fragment_size;
    std::size_t sampling_interval;
    global_data_config storage_conf;
    set_config set_conf;
};



} // end namespace uh::cluster

#endif //CORE_CLUSTER_CONFIG_H
