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

#include "common/db/config.h"
#include "common/network/server.h"
#include "deduplicator/config.h"
#include "storage/global/config.h"

namespace uh::cluster {

struct entrypoint_config {
    std::size_t num_threads = 4;

    server_config server = {
        .port = 8080,
        .bind_address = "0.0.0.0",
    };

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
