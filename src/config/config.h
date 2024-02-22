#ifndef CONFIG_CONFIG_H
#define CONFIG_CONFIG_H

#include "common/utils/cluster_config.h"
#include "common/telemetry/log.h"

namespace uh::cluster {

struct config {
    cluster::role role;
    service_config service;
    uh::log::config log;
};

config read_config(int argc, char** argv);

} // namespace uh::cluster

#endif
