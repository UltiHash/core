#pragma once

#include <common/network/server.h>

namespace uh::cluster::proxy {

struct config {
    server_config server = {
        .port = 8088,
        .bind_address = "0.0.0.0"
    };

    std::string downstream_host;
    uint16_t downstream_port;
    std::size_t connections = 16;
    std::size_t num_threads = 4;
    std::size_t buffer_size = 64 * KIBI_BYTE;
};

} // namespace uh::cluster::proxy
