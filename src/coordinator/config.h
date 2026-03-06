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

#include <common/db/config.h>
#include <common/license/backend_client.h>
#include <common/license/license.h>
#include <storage/group/config.h>

namespace uh::cluster {

struct coordinator_config {
    std::size_t num_threads = 2;

    uh::cluster::license license;
    storage::group_configs storage_groups;
    default_backend_client::config backend_config;
    db::config database_config;
};

} // end namespace uh::cluster
