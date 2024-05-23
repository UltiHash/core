#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include "common/db/config.h"
#include "common/license/license.h"
#include "common/telemetry/log.h"
#include "common/types/big_int.h"
#include "common/utils/common.h"
#include "deduplicator/config.h"
#include "entrypoint/config.h"
#include "storage/config.h"
#include <filesystem>
#include <optional>

#include <CLI/CLI.hpp>

namespace uh::cluster {

struct service_config {
    std::string etcd_url = "http://127.0.0.1:2379";
    std::filesystem::path working_dir = "/var/lib/uh";
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
};

std::optional<config> read_config(int argc, char** argv);

void configure(CLI::App& app, db::config& cfg);

} // namespace uh::cluster

#endif
