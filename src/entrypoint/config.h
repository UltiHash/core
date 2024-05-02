#ifndef CORE_ENTRYPOINT_CONFIG_H
#define CORE_ENTRYPOINT_CONFIG_H

#include "common/network/server.h"
#include "deduplicator/config.h"
#include "directory/config.h"

namespace uh::cluster {

struct entrypoint_config {
    server_config server = {
        .threads = 4, .port = 8080, .bind_address = "0.0.0.0"};

    std::size_t dedupe_node_connection_count = 16ul;
    std::size_t directory_connection_count = 4ul;
    std::size_t worker_thread_count = 16ul;
    std::size_t buffer_size = 64ul * MEBI_BYTE;
    std::optional<deduplicator_config> m_attached_deduplicator;
    std::optional<directory_config> m_attached_directory;

};

} // namespace uh::cluster

#endif
