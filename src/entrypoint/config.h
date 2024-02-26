#ifndef CORE_ENTRYPOINT_CONFIG_H
#define CORE_ENTRYPOINT_CONFIG_H

#include "common/network/server.h"


namespace uh::cluster {

struct entrypoint_config {
    server_config server = {
        .threads = 4, .port = 8080, .bind_address = "0.0.0.0"};

    std::size_t dedupe_node_connection_count = 4ul;
    std::size_t directory_connection_count = 4ul;
    std::size_t worker_thread_count = 12ul;
};

} // namespace uh::cluster

#endif
