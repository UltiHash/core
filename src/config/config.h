#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include "common/license/license.h"
#include "common/types/big_int.h"
#include "common/utils/common.h"
#include "common/telemetry/log.h"
#include <filesystem>


namespace uh::cluster {

struct service_config {
    std::string etcd_url;
    std::string telemetry_url;
    std::filesystem::path working_dir;
    uh::cluster::license license;
};

struct config {
    cluster::role role;
    service_config service;
    uh::log::config log;
};

config read_config(int argc, char** argv);

} // namespace uh::cluster

#endif
