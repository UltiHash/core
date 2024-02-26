#ifndef CORE_CLUSTER_CONFIG_H
#define CORE_CLUSTER_CONFIG_H

#include "common.h"
#include "common/license/license.h"
#include "common/types/big_int.h"
#include <filesystem>

namespace uh::cluster {

struct service_config {
    std::string etcd_url;
    std::string telemetry_url;
    std::filesystem::path working_dir;
    uh::cluster::license license;
};

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

struct deduplicator_config {
    std::filesystem::path working_dir;
    std::size_t min_fragment_size{};
    std::size_t max_fragment_size{};
    std::size_t dedupe_worker_minimum_data_size{};
    std::size_t worker_thread_count{};
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
