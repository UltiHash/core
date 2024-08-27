#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include "common/types/common_types.h"
#include <map>
#include <string>
#include <unordered_set>

namespace uh::cluster {

enum role : uint8_t {
    STORAGE_SERVICE,
    DEDUPLICATOR_SERVICE,
    ENTRYPOINT_SERVICE,
};

const std::map<std::string, role> role_by_abbreviation = {
    {"storage", STORAGE_SERVICE},
    {"deduplicator", DEDUPLICATOR_SERVICE},
    {"entrypoint", ENTRYPOINT_SERVICE}};

enum message_type : uint8_t {
    STORAGE_READ_FRAGMENT_REQ = 0,
    STORAGE_READ_ADDRESS_REQ = 1,
    STORAGE_READ_REQ = 8,
    STORAGE_WRITE_REQ = 2,
    STORAGE_SYNC_REQ = 3,
    STORAGE_LINK_REQ = 9,
    STORAGE_UNLINK_REQ = 4,
    STORAGE_USED_REQ = 5,
    DEDUPLICATOR_REQ = 6,

    SUCCESS = 15,
    FAILURE = 16
};

constexpr const char* ENV_CFG_ENDPOINT_HOST = "UH_POD_IP";
constexpr const char* UH_WORKING_DIR = "UH_WORKING_DIR";
constexpr const char* ENV_CFG_LOG_LEVEL = "UH_LOG_LEVEL";
constexpr const char* ENV_CFG_LICENSE = "UH_LICENSE";
constexpr const char* ENV_CFG_OTEL_ENDPOINT = "UH_OTEL_ENDPOINT";
constexpr const char* ENV_CFG_OTEL_EXPORT_INTERVAL = "UH_OTEL_INTERVAL";
constexpr const char* ENV_CFG_DB_HOSTPORT = "UH_DB_HOSTPORT";
constexpr const char* ENV_CFG_DB_DIRECTORY_CONNECTIONS =
    "UH_DB_DIRECTORY_CONNECTIONS";
constexpr const char* ENV_CFG_DB_MULTIPART_CONNECTIONS =
    "UH_DB_MULTIPART_CONNECTIONS";
constexpr const char* ENV_CFG_DB_USER = "UH_DB_USER";
constexpr const char* ENV_CFG_DB_PASS = "UH_DB_PASS";

constexpr const char* RESERVED_BUCKET_NAME = "ultihash";

constexpr auto SERVICE_GET_TIMEOUT = std::chrono::seconds(10);

constexpr auto ETCD_TIMEOUT = std::chrono::seconds(300);
constexpr auto ETCD_RETRY_INTERVAL = std::chrono::seconds(1);

constexpr std::string_view CONFIG_PATH_DELIMETER = ":";

constexpr size_t SET_LOG_CACHE_SIZE = 10000;
constexpr size_t INPUT_CHUNK_SIZE = 64ul * MEBI_BYTE;

constexpr std::size_t DEFAULT_PAGE_SIZE = 8 * KIBI_BYTE;

const std::string& get_service_string(const role& service_role);

} // end namespace uh::cluster

#endif // CORE_COMMON_H
