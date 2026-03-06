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
#include <common/etcd/service.h>
#include <common/telemetry/log.h>
#include <common/utils/common.h>
#include <coordinator/config.h>
#include <deduplicator/config.h>
#include <entrypoint/config.h>
#include <storage/config.h>
#include <proxy/config.h>

#include <CLI/CLI.hpp>
#include <optional>

namespace uh::cluster {

struct config {
    cluster::role role;
    service_config service;
    uh::log::config log;

    entrypoint_config entrypoint;
    storage_config storage;
    deduplicator_config deduplicator;
    coordinator_config coordinator;
    proxy::config proxy;
};

std::optional<config> read_config(int argc, char** argv);

void configure(CLI::App& app, db::config& cfg);
void configure(CLI::App& app, boost::log::trivial::severity_level& log_level);

} // namespace uh::cluster
