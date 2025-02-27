#pragma once

#include <common/etcd/utils.h>

#include <string>

struct service_config {
    uh::cluster::etcd_config etcd_config;
    std::string working_dir = "/var/lib/uh";
    std::string telemetry_url;
    unsigned telemetry_interval = 1000;
    bool enable_traces = false;
};
