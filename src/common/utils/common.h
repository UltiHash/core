#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include "common/types/common_types.h"
#include <map>
#include <string>
#include <unordered_set>
#include <vector>

namespace uh::cluster {

enum role : uint8_t {
    STORAGE_SERVICE,
    DEDUPLICATOR_SERVICE,
    ENTRYPOINT_SERVICE,
};

const std::map<std::string, uh::cluster::role> role_by_abbreviation = {
    {"storage", STORAGE_SERVICE},
    {"deduplicator", DEDUPLICATOR_SERVICE},
    {"entrypoint", ENTRYPOINT_SERVICE}};

enum message_type : uint8_t {
    STORAGE_READ_FRAGMENT_REQ = 0,
    STORAGE_READ_ADDRESS_REQ = 1,
    STORAGE_WRITE_REQ = 2,
    STORAGE_SYNC_REQ = 3,
    STORAGE_USED_REQ = 5,
    STORAGE_AVAILABLE_REQ = 7,

    DEDUPLICATOR_REQ = 6,

    SUCCESS = 15,
    FAILURE = 16
};

enum config_parameter {
    CFG_ENDPOINT_HOST,
    CFG_ENDPOINT_PORT,
    CFG_ENDPOINT_PID,
};

constexpr std::array<std::pair<uh::cluster::config_parameter, const char*>, 3>
    string_by_config_parameter = {{
        {uh::cluster::CFG_ENDPOINT_HOST, "endpoint_host"},
        {uh::cluster::CFG_ENDPOINT_PORT, "endpoint_port"},
        {uh::cluster::CFG_ENDPOINT_PID, "endpoint_pid"},
    }};

static constexpr const char* ENV_CFG_ENDPOINT_HOST = "UH_POD_IP";
static constexpr const char* ENV_CFG_LOG_LEVEL = "UH_LOG_LEVEL";
static constexpr const char* ENV_CFG_LICENSE = "UH_LICENSE";
static constexpr const char* ENV_CFG_OTEL_ENDPOINT = "UH_OTEL_ENDPOINT";
static constexpr const char* ENV_CFG_OTEL_EXPORT_INTERVAL = "UH_OTEL_INTERVAL";
static constexpr const char* ENV_CFG_DB_HOSTPORT = "UH_DB_HOSTPORT";
static constexpr const char* ENV_CFG_DB_USER = "UH_DB_USER";
static constexpr const char* ENV_CFG_DB_PASS = "UH_DB_PASS";

static constexpr const char* RESERVED_BUCKET_NAME = "ultihash";

static constexpr int ETCD_TIMEOUT = 60;
static constexpr int ETCD_RETRY_INTERVAL = 1;

static constexpr size_t SET_LOG_CACHE_SIZE = 10000;

const std::string& get_service_string(const uh::cluster::role& service_role);

constexpr const char*
get_config_string(const uh::cluster::config_parameter& cfg_param) {
    for (const auto& entry : string_by_config_parameter) {
        if (entry.first == cfg_param)
            return entry.second;
    }

    throw std::invalid_argument("invalid configuration parameter");
}

} // end namespace uh::cluster

#endif // CORE_COMMON_H
