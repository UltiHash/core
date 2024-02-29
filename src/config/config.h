#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include "common/license/license.h"
#include "common/types/big_int.h"
#include "common/utils/common.h"
#include "common/telemetry/log.h"
#include "deduplicator/config.h"
#include "directory/config.h"
#include "entrypoint/config.h"
#include "storage/config.h"
#include <filesystem>
#include <optional>


namespace uh::cluster {

struct service_config {
    std::string etcd_url = "http://127.0.0.1:2379";
    std::filesystem::path working_dir = "/var/lib/uh";
    std::string telemetry_url;
    uh::cluster::license license;
};

struct config {
    cluster::role role;
    service_config service;
    uh::log::config log;

    entrypoint_config entrypoint;
    storage_config storage;
    directory_config directory;
    deduplicator_config deduplicator;
};

std::optional<config> read_config(int argc, char** argv);

} // namespace uh::cluster

#endif
