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

// fundamental config

struct server_config
{
    int threads;
    uint16_t port;
};

struct growing_plain_storage_config {
    std::filesystem::path file;
    size_t init_size;
};

struct set_config {
    unsigned long set_minimum_free_space{};
    unsigned long max_empty_hole_size{};
    growing_plain_storage_config key_store_config;
};

// roles config

struct dedupe_config {
    std::size_t min_fragment_size{};
    std::size_t max_fragment_size{};
    std::size_t sampling_interval{};
    server_config server_conf{};
    int data_node_connection_count{};
    set_config set_conf;
};

struct data_node_config {
    std::filesystem::path directory;
    std::filesystem::path hole_log;
    size_t min_file_size;
    size_t max_file_size;
    uint128_t max_data_store_size;
    server_config server_conf;
};

struct entry_node_config {
    server_config internal_server_conf;
    server_config rest_server_conf;
    int dedupe_node_connection_count;
    int directory_connection_count;
};

struct bucket_config {
    size_t min_file_size;
    size_t max_file_size;
    size_t max_storage_size;
    size_t max_chunk_size;
};

struct directory_store_config {
    std::filesystem::path root;
    bucket_config bucket_conf;
};

struct directory_node_config {
    server_config server_conf{};
    directory_store_config directory_conf;
    int data_node_connection_count{};
};

struct cluster_config {
    int init_process_count{};
    data_node_config data_node_conf;
    dedupe_config dedupe_node_conf;
    directory_node_config directory_node_conf;
    entry_node_config entry_node_conf{};
};

} // end namespace uh::cluster

#endif //CORE_CLUSTER_CONFIG_H
