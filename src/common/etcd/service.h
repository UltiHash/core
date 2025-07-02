#pragma once

#include <common/etcd/utils.h>

#include <string>

namespace uh::cluster {

struct service_config {
    std::size_t threads = 4;
    uh::cluster::etcd_config etcd_config;
    std::filesystem::path working_dir = "/var/lib/uh";
    std::string telemetry_url;
    unsigned telemetry_interval = 1000;
    bool enable_traces = false;
};

} // namespace uh::cluster
