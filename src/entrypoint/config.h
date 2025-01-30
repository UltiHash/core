#pragma once

#include <common/db/config.h>
#include <common/network/server.h>
#include <deduplicator/config.h>

namespace uh::cluster {

enum class dd_type { local, cluster, null };

enum class sn_type { local, cluster, null };

struct entrypoint_config {
    server_config server = {
        .threads = 4, .port = 8080, .bind_address = "0.0.0.0"};

    std::size_t worker_thread_count = 16ul;
    std::size_t buffer_size = INPUT_CHUNK_SIZE;

    dd_type deduplicator = dd_type::cluster;
    deduplicator_config local_deduplicator;
    std::size_t dedupe_node_connection_count = 16ul;

    sn_type storage = sn_type::cluster;
    global_data_view_config cluster_storage;
    storage_config local_storage;

    db::config database;
};

} // namespace uh::cluster
