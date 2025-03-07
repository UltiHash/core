#pragma once

#include "common/db/config.h"
#include "common/etcd/utils.h"
#include "common/telemetry/log.h"
#include "common/utils/common.h"
#include "coordinator/config.h"
#include "deduplicator/config.h"
#include "entrypoint/config.h"
#include "storage/config.h"

#include <CLI/CLI.hpp>
#include <optional>

namespace uh::cluster {

struct service_config {
    uh::cluster::etcd_config etcd_config;
    std::string working_dir = "/var/lib/uh";
    std::string telemetry_url;
    unsigned telemetry_interval = 1000;
    bool enable_traces = false;
};

struct config {
    cluster::role role;
    service_config service;
    uh::log::config log;

    entrypoint_config entrypoint;
    storage_config storage;
    deduplicator_config deduplicator;
    coordinator_config coordinator;
};

std::optional<config> read_config(int argc, char** argv);

void configure(CLI::App& app, db::config& cfg);
void configure(CLI::App& app, boost::log::trivial::severity_level& log_level);

} // namespace uh::cluster
