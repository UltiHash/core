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
#include <storage/config.h>
#include <storage/global/config.h>

#include <filesystem>

namespace uh::cluster {

constexpr std::size_t PREFIX_SIZE = 16;

struct deduplicator_config {
    std::size_t num_threads = 10;

    server_config server = {
        .port = 9300,
        .bind_address = "0.0.0.0",
    };

    global_data_view_config global_data_view;
    std::filesystem::path working_dir = "/var/lib/uh/deduplicator";
    std::size_t min_fragment_size = 32ul;
    std::size_t max_fragment_size = DEFAULT_PAGE_SIZE;
    std::size_t worker_thread_count = 16ul;
    std::size_t set_capacity = 1000000;
    std::optional<storage_config> m_attached_storage;
};

} // namespace uh::cluster
