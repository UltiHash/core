#ifndef CORE_CLUSTER_CONFIG_H
#define CORE_CLUSTER_CONFIG_H

#include "common.h"
#include "common/types/big_int.h"
#include <filesystem>

namespace uh::cluster {

// fundamental config

struct server_config {
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
    std::filesystem::path working_dir;
    bucket_config bucket_conf;
};

struct global_data_view_config {
    std::size_t storage_service_connection_count{};
    std::size_t read_cache_capacity_l1{};
    std::size_t read_cache_capacity_l2{};
    std::size_t l1_sample_size{};
    uint128_t max_data_store_size;
};

// roles config

struct deduplicator_config {
    std::filesystem::path working_dir;
    std::size_t min_fragment_size{};
    std::size_t max_fragment_size{};
    std::size_t dedupe_worker_minimum_data_size{};
    std::size_t worker_thread_count{};
};

struct storage_config {
    std::filesystem::path working_dir;
    std::size_t min_file_size;
    std::size_t max_file_size;
    uint128_t max_data_store_size;
};

struct entrypoint_config {
    std::size_t dedupe_node_connection_count{};
    std::size_t directory_connection_count{};
    std::size_t worker_thread_count{};
};

struct directory_config {
    directory_store_config directory_store_conf;
    std::size_t worker_thread_count{};
    uint128_t max_data_store_size = 4 * TERA_BYTE;
};

} // end namespace uh::cluster

#endif // CORE_CLUSTER_CONFIG_H
