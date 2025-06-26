#pragma once

#include "common/db/config.h"
#include "common/network/server.h"
#include "deduplicator/config.h"
#include "storage/global/config.h"

namespace uh::cluster {

struct entrypoint_config {
    server_config server = {
        .threads = 4, .port = 8080, .bind_address = "0.0.0.0"};

    bool noop_deduplicator = false;
    std::size_t dedupe_node_connection_count = 10ul;
    std::size_t worker_thread_count = 16ul;
    std::size_t buffer_size = INPUT_CHUNK_SIZE;
    std::optional<deduplicator_config> m_attached_deduplicator;
    std::optional<storage_config> m_attached_storage;
    db::config database;
    global_data_view_config global_data_view;
};

} // namespace uh::cluster
