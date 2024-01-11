//
// Created by masi on 7/17/23.
//

#ifndef CORE_CLUSTER_CONFIG_H
#define CORE_CLUSTER_CONFIG_H

#include <vector>
#include <numeric>
#include <filesystem>
#include "big_int.h"

namespace uh::cluster {

// fundamental config

struct server_config
{
    int threads;
    uint16_t port;
    std::string address;
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
};

// roles config

struct deduplicator_config {
    std::size_t min_fragment_size{};
    std::size_t max_fragment_size{};
    server_config server_conf{};
    int data_node_connection_count{};
    std::filesystem::path set_log_path;
    size_t dedupe_worker_minimum_data_size{};
    int worker_thread_count {};
};

struct storage_config {
    std::filesystem::path directory;
    std::filesystem::path hole_log;
    size_t min_file_size;
    size_t max_file_size;
    uint128_t max_data_store_size;
    server_config server_conf;
};

struct entrypoint_config {
    server_config rest_server_conf;
    int dedupe_node_connection_count {};
    int directory_connection_count {};
    int worker_thread_count {};
};

struct directory_config {
    server_config server_conf{};
    directory_store_config directory_conf;
    int data_node_connection_count{};
    int worker_thread_count {};
};

struct cluster_config {
    storage_config storage_conf;
    deduplicator_config deduplicator_conf;
    directory_config directory_conf;
    entrypoint_config entrypoint_conf{};
    global_data_view_config global_data_view_conf;
};

entrypoint_config make_entrypoint_config ();

directory_config make_directory_config ();

deduplicator_config make_deduplicator_config ();

storage_config make_storage_config ();

global_data_view_config make_global_data_view_config ();;

} // end namespace uh::cluster

#endif //CORE_CLUSTER_CONFIG_H
