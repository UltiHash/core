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

#include <common/etcd/utils.h>

#include <string>

namespace uh::cluster {

struct service_config {
    uh::cluster::etcd_config etcd_config;
    std::filesystem::path working_dir = "/var/lib/uh";
    std::string telemetry_url;
    unsigned telemetry_interval = 1000;
    bool enable_traces = false;
};

} // namespace uh::cluster
