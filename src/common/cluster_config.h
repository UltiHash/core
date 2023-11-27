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
    std::string metrics_bind_address;
    std::size_t metrics_threads;
    std::string metrics_path;
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

struct global_data_view_config {
    int read_cache_capacity_l1 {};
    int read_cache_capacity_l2 {};
    size_t l1_sample_size {};
    ec_type ec_algorithm {};
    size_t recovery_chunk_size {};
};

// roles config

struct dedupe_config {
    std::size_t min_fragment_size{};
    std::size_t max_fragment_size{};
    server_config server_conf{};
    int data_node_connection_count{};
    std::filesystem::path set_log_path;
    size_t dedupe_worker_minimum_data_size{};
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

struct directory_node_config {
    server_config server_conf{};
    directory_store_config directory_conf;
    int data_node_connection_count{};
};

struct cluster_config {
    int init_process_count {};
    data_node_config data_node_conf;
    dedupe_config dedupe_node_conf;
    directory_node_config directory_node_conf;
    entry_node_config entry_node_conf{};
    global_data_view_config global_data_view_conf;
};

} // end namespace uh::cluster

#endif //CORE_CLUSTER_CONFIG_H
