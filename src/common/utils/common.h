#pragma once

#include <chrono>
#include <string>

namespace uh::cluster {

static constexpr std::size_t KIBI_BYTE = 1024;
static constexpr std::size_t MEBI_BYTE = 1024 * KIBI_BYTE;
static constexpr std::size_t GIBI_BYTE = 1024 * MEBI_BYTE;
static constexpr std::size_t TEBI_BYTE = 1024 * GIBI_BYTE;
static constexpr std::size_t PEBI_BYTE = 1024 * TEBI_BYTE;

enum role : uint8_t {
    STORAGE_SERVICE,
    DEDUPLICATOR_SERVICE,
    ENTRYPOINT_SERVICE,
    COORDINATOR_SERVICE
};

inline role global_service_role;

enum message_type : uint8_t {

    SUCCESS = 0,
    FAILURE = 1,

    STORAGE_READ_ADDRESS_REQ = 33,
    STORAGE_READ_REQ = 34,
    STORAGE_WRITE_REQ = 35,
    STORAGE_LINK_REQ = 37,
    STORAGE_UNLINK_REQ = 38,
    STORAGE_USED_REQ = 39,
    STORAGE_UPDATE_STATE_REQ = 40,

    DEDUPLICATOR_REQ = 64,

};

constexpr const char* ENV_CFG_ENDPOINT_HOST = "UH_POD_IP";
constexpr const char* UH_WORKING_DIR = "UH_WORKING_DIR";
constexpr const char* ENV_CFG_LOG_LEVEL = "UH_LOG_LEVEL";
constexpr const char* ENV_CFG_LICENSE = "UH_LICENSE";
constexpr const char* ENV_CFG_STORAGE_GROUPS = "UH_STORAGE_GROUPS";
constexpr const char* ENV_CFG_BACKEND_HOST = "UH_BACKEND_HOST";
constexpr const char* ENV_CFG_CUSTOMER_ID = "UH_CUSTOMER_ID";
constexpr const char* ENV_CFG_ACCESS_TOKEN = "UH_ACCESS_TOKEN";
constexpr const char* ENV_CFG_OTEL_ENDPOINT = "UH_OTEL_ENDPOINT";
constexpr const char* ENV_CFG_OTEL_EXPORT_INTERVAL = "UH_OTEL_INTERVAL";
constexpr const char* ENV_CFG_ENABLE_TRACES = "UH_TRACES_ENABLED";
constexpr const char* ENV_CFG_DB_HOSTPORT = "UH_DB_HOSTPORT";
constexpr const char* ENV_CFG_DB_DIRECTORY_CONNECTIONS =
    "UH_DB_DIRECTORY_CONNECTIONS";
constexpr const char* ENV_CFG_DB_MULTIPART_CONNECTIONS =
    "UH_DB_MULTIPART_CONNECTIONS";
constexpr const char* ENV_CFG_DB_USERS_CONNECTIONS = "UH_DB_USERS_CONNECTIONS";
constexpr const char* ENV_CFG_DB_USER = "UH_DB_USER";
constexpr const char* ENV_CFG_DB_PASS = "UH_DB_PASS";
constexpr const char* ENV_CFG_ETCD_USERNAME = "UH_ETCD_USERNAME";
constexpr const char* ENV_CFG_ETCD_PASSWORD = "UH_ETCD_PASSWORD";
constexpr const char* ENV_CFG_NO_DEDUPE = "UH_NO_DEDUPE";
constexpr const char* ENV_CFG_STORAGE_SERVICE_ID = "UH_STORAGE_SERVICE_ID";

constexpr const char* RESERVED_BUCKET_NAME = "ultihash";

constexpr auto SERVICE_GET_TIMEOUT = std::chrono::seconds(10);

constexpr auto LICENSE_FETCH_PERIOD = std::chrono::hours(1);

constexpr std::string_view CONFIG_PATH_DELIMETER = ":";

constexpr size_t SET_LOG_CACHE_SIZE = 10000;
constexpr size_t INPUT_CHUNK_SIZE = 64ul * MEBI_BYTE;

constexpr std::size_t DEFAULT_PAGE_SIZE = 8 * KIBI_BYTE;

const std::string& get_service_string(const role& service_role);

} // end namespace uh::cluster
