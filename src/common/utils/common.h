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
    DIRECTORY_SERVICE,
    ENTRYPOINT_SERVICE,
};

const std::map<std::string, uh::cluster::role> role_by_abbreviation = {
    {"storage", STORAGE_SERVICE},
    {"deduplicator", DEDUPLICATOR_SERVICE},
    {"directory", DIRECTORY_SERVICE},
    {"entrypoint", ENTRYPOINT_SERVICE}};

enum message_type : uint8_t {
    STORAGE_READ_FRAGMENT_REQ = 0,
    STORAGE_READ_ADDRESS_REQ = 1,
    STORAGE_WRITE_REQ = 2,
    STORAGE_SYNC_REQ = 3,
    STORAGE_REMOVE_FRAGMENT_REQ = 4,
    STORAGE_USED_REQ = 5,

    DEDUPLICATOR_REQ = 6,

    DIRECTORY_BUCKET_LIST_REQ = 7,
    DIRECTORY_BUCKET_PUT_REQ = 8,
    DIRECTORY_BUCKET_DELETE_REQ = 9,
    DIRECTORY_BUCKET_EXISTS_REQ = 10,

    DIRECTORY_OBJECT_LIST_REQ = 11,
    DIRECTORY_OBJECT_PUT_REQ = 12,
    DIRECTORY_OBJECT_GET_REQ = 13,
    DIRECTORY_OBJECT_DELETE_REQ = 14,

    SUCCESS = 15,
    FAILURE = 16
};

enum config_parameter {
    CFG_ENDPOINT_HOST,
    CFG_ENDPOINT_PORT,
};

constexpr std::array<std::pair<uh::cluster::config_parameter, const char*>, 2>
    string_by_config_parameter = {{
        {uh::cluster::CFG_ENDPOINT_HOST, "endpoint_host"},
        {uh::cluster::CFG_ENDPOINT_PORT, "endpoint_port"},
    }};

static constexpr const char* ENV_CFG_ENDPOINT_HOST = "UH_POD_IP";
static constexpr const char* ENV_CFG_LOG_LEVEL = "UH_LOG_LEVEL";
static constexpr const char* ENV_CFG_LICENSE = "UH_LICENSE";
static constexpr const char* ENV_CFG_OTEL_ENDPOINT = "UH_OTEL_ENDPOINT";
static constexpr const char* ENV_CFG_OTEL_EXPORT_INTERVAL = "UH_OTEL_INTERVAL";

static constexpr int ETCD_TIMEOUT = 60;
static constexpr int ETCD_RETRY_INTERVAL = 1;
static constexpr size_t DATASTORE_MAX_SIZE = 1 * PEBI_BYTE;

uh::cluster::role get_service_role(const std::string& service_role_str);
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
