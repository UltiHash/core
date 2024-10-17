#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include "common/db/config.h"
#include "common/etcd/utils.h"
#include "common/license/license.h"
#include "common/telemetry/log.h"
#include "common/utils/common.h"
#include "deduplicator/config.h"
#include "entrypoint/config.h"
#include "storage/config.h"
#include <optional>

#include <CLI/CLI.hpp>
#include <recovery/config.h>

namespace uh::cluster {

struct service_config {
    uh::cluster::etcd_config etcd_config;
    std::string working_dir = "/var/lib/uh";
    std::string telemetry_url;
    uh::cluster::license license;
    unsigned telemetry_interval = 1000;
};

struct config {
    cluster::role role;
    service_config service;
    uh::log::config log;

    entrypoint_config entrypoint;
    storage_config storage;
    deduplicator_config deduplicator;
    recovery_config recovery;
};

std::optional<config> read_config(int argc, char** argv);

void configure(CLI::App& app, db::config& cfg);
void configure(CLI::App& app, boost::log::trivial::severity_level& log_level);

} // namespace uh::cluster

#endif
