//
// Created by massi on 1/11/24.
//
#include "cluster_config.h"

namespace uh::cluster {

entrypoint_config make_entrypoint_config() {
    return {
            .rest_server_conf = {
                    .threads = 4,
                    .port = 8080,
                    .address = "0.0.0.0",
                    .metrics_bind_address = "0.0.0.0:9080",
                    .metrics_threads = 2,
                    .metrics_path = "/metrics"
            },
            .dedupe_node_connection_count = 4,
            .directory_connection_count = 4,
            .worker_thread_count = 12,
    };
}

directory_config make_directory_config() {
    return {
            .server_conf = {
                    .threads = 4,
                    .port = 8083,
                    .address = "0.0.0.0",
                    .metrics_bind_address = "0.0.0.0:9083",
                    .metrics_threads = 2,
                    .metrics_path = "/metrics"
            },
            .directory_conf = {
                    .root = "ultihash-root/dr",
                    .bucket_conf = {
                            .min_file_size = 1024ul * 1024ul * 1024ul * 2,
                            .max_file_size = 1024ul * 1024ul * 1024ul * 64,
                            .max_storage_size = 1024ul * 1024ul * 1024ul * 256,
                            .max_chunk_size = std::numeric_limits <uint32_t>::max(),
                    },
            },
            .data_node_connection_count = 4,
            .worker_thread_count = 8,
    };
}

deduplicator_config make_deduplicator_config() {
    return {
            .min_fragment_size = 32,
            .max_fragment_size = 8 * 1024,
            .server_conf = {
                    .threads = 4,
                    .port = 8084,
                    .address = "0.0.0.0",
                    .metrics_bind_address = "0.0.0.0:9084",
                    .metrics_threads = 2,
                    .metrics_path = "/metrics"
            },
            .data_node_connection_count = 16,
            .set_log_path = "ultihash-root/dd/set_log",
            .dedupe_worker_minimum_data_size = 128ul * 1024ul,
            .worker_thread_count = 32,
    };
}

storage_config make_storage_config() {
    return {
            .directory = "ultihash-root/dn",
            .hole_log = "ultihash-root/dn/log",
            .min_file_size = 1ul * 1024ul * 1024ul * 1024ul,
            .max_file_size = 4ul * 1024ul * 1024ul * 1024ul,
            .max_data_store_size = 64ul * 1024ul * 1024ul * 1024ul,
            .server_conf = {
                    .threads = 16,
                    .port = 8082,
                    .address = "0.0.0.0",
                    .metrics_bind_address = "0.0.0.0:9082",
                    .metrics_threads = 2,
                    .metrics_path = "/metrics"
            },
    };
}

global_data_view_config make_global_data_view_config() {

    return {
            .read_cache_capacity_l1= 8000000,
            .read_cache_capacity_l2= 4000,
            .l1_sample_size = 128,
    };
}
}