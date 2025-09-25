#pragma once

#include <common/network/server.h>
#include <storage/global/config.h>

namespace uh::cluster::proxy {

struct config {
    server_config server = {.port = 8088, .bind_address = "0.0.0.0"};

    bool downstream_insecure;
    std::optional<std::string> downstream_cert_file;
    std::string downstream_host;
    uint16_t downstream_port;
    std::size_t connections = 16;
    std::size_t num_threads = 4;
    std::size_t buffer_size = 64 * KIBI_BYTE;
    global_data_view_config gdv;
};

} // namespace uh::cluster::proxy
