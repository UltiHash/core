#ifndef CORE_CLUSTER_CONFIG_H
#define CORE_CLUSTER_CONFIG_H

#include "common.h"
#include "common/license/license.h"
#include "common/types/big_int.h"
#include <filesystem>

namespace uh::cluster {

struct service_config {
    std::string etcd_url;
    std::string telemetry_url;
    std::filesystem::path working_dir;
    uh::cluster::license license;
};

struct server_config {
    std::size_t threads;
    uint16_t port;
    std::string bind_address;
};

} // end namespace uh::cluster

#endif // CORE_CLUSTER_CONFIG_H
