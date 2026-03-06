/*
 * Copyright 2026 UltiHash Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

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
