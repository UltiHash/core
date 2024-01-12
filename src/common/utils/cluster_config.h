//
// Created by masi on 7/17/23.
//

#ifndef CORE_CLUSTER_CONFIG_H
#define CORE_CLUSTER_CONFIG_H

#include <vector>
#include <numeric>
#include <filesystem>
#include "common/utils/common.h"

namespace uh::cluster {

// fundamental config

struct server_config
{
    std::size_t threads;
    uint16_t port;
    std::string bind_address;
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

struct deduplicator_config {
    std::size_t min_fragment_size{};
    std::size_t max_fragment_size{};
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
};

struct entrypoint_config {
    int dedupe_node_connection_count {};
    int directory_connection_count {};
    int worker_thread_count {};
};

struct directory_config {
    directory_store_config directory_conf;
    int data_node_connection_count{};
    int worker_thread_count {};
};

struct cluster_config {
    int init_process_count {};
    storage_config storage_conf;
    deduplicator_config deduplicator_conf;
    directory_config directory_conf;
    entrypoint_config entrypoint_conf{};
    global_data_view_config global_data_view_conf;
};

static entrypoint_config make_entrypoint_config () {
    return {
            .dedupe_node_connection_count = 4,
            .directory_connection_count = 4,
            .worker_thread_count = 12,
    };
}

static bucket_config make_bucket_config() {
    return {
        .min_file_size = 1024ul * 1024ul * 1024ul * 2,
        .max_file_size = 1024ul * 1024ul * 1024ul * 64,
        .max_storage_size = 1024ul * 1024ul * 1024ul * 256,
        .max_chunk_size = std::numeric_limits<uint32_t>::max(),
    };
}

static directory_config make_directory_config () {
    return {
            .data_node_connection_count = 4,
            .worker_thread_count = 8,
    };
}

static deduplicator_config make_deduplicator_config () {
    return {
            .min_fragment_size = 32,
            .max_fragment_size = 8 * 1024,
            .data_node_connection_count = 16,
            .set_log_path = "ultihash-root/dd/set_log",
            .dedupe_worker_minimum_data_size = 128ul * 1024ul,
            .worker_thread_count = 32,
    };
}

static storage_config make_storage_config () {
    return {
            .directory = "ultihash-root/dn",
            .hole_log = "ultihash-root/dn/log",
            .min_file_size = 1ul * 1024ul * 1024ul * 1024ul,
            .max_file_size = 4ul * 1024ul * 1024ul * 1024ul,
            .max_data_store_size = 64ul * 1024ul * 1024ul * 1024ul,
    };
}

static global_data_view_config make_global_data_view_config () {

    return {
            .read_cache_capacity_l1= 8000000,
            .read_cache_capacity_l2= 4000,
            .l1_sample_size = 128,
            .ec_algorithm = uh::cluster::NONE,
            .recovery_chunk_size = 1024ul * 1024ul * 1024ul,
    };
};

} // end namespace uh::cluster

#endif //CORE_CLUSTER_CONFIG_H
