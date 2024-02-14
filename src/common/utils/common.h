#ifndef CORE_COMMON_H
#define CORE_COMMON_H
#include "common/types/common_types.h"
#include <map>
#include <string>
#include <vector>

namespace uh::cluster {

enum message_type : uint8_t {
    STORAGE_READ_FRAGMENT_REQ = 10,
    STORAGE_READ_FRAGMENT_RESP = 11,
    STORAGE_READ_ADDRESS_REQ = 43,
    STORAGE_READ_ADDRESS_RESP = 44,
    STORAGE_WRITE_REQ = 12,
    STORAGE_WRITE_RESP = 13,
    STORAGE_SYNC_REQ = 20,
    STORAGE_REMOVE_FRAGMENT_REQ = 22,
    STORAGE_USED_REQ = 24,
    STORAGE_USED_RESP = 25,

    DEDUPLICATOR_REQ = 26,
    DEDUPLICATOR_RESP = 27,

    DIRECTORY_BUCKET_LIST_REQ = 37,
    DIRECTORY_BUCKET_LIST_RESP = 38,
    DIRECTORY_BUCKET_PUT_REQ = 31,
    DIRECTORY_BUCKET_DELETE_REQ = 41,
    DIRECTORY_BUCKET_EXISTS = 46,

    DIRECTORY_OBJECT_LIST_REQ = 39,
    DIRECTORY_OBJECT_LIST_RESP = 40,
    DIRECTORY_OBJECT_PUT_REQ = 28,
    DIRECTORY_OBJECT_GET_REQ = 29,
    DIRECTORY_OBJECT_GET_RESP = 30,
    DIRECTORY_OBJECT_DELETE_REQ = 45,

    SUCCESS = 32,
    FAILURE = 33,
};

enum role : uint8_t {
    STORAGE_SERVICE,
    DEDUPLICATOR_SERVICE,
    DIRECTORY_SERVICE,
    ENTRYPOINT_SERVICE,
};

enum config_parameter {
    CFG_SERVER_THREADS,
    CFG_SERVER_BIND_ADDR,
    CFG_SERVER_PORT,

    CFG_ENDPOINT_HOST,
    CFG_ENDPOINT_PORT,

    CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT,
    CFG_GDV_READ_CACHE_CAPACITY_L1,
    CFG_GDV_READ_CACHE_CAPACITY_L2,
    CFG_GDV_L1_SAMPLE_SIZE,
    CFG_GDV_MAX_DATA_STORE_SIZE,

    CFG_STORAGE_MIN_FILE_SIZE,
    CFG_STORAGE_MAX_FILE_SIZE,
    CFG_STORAGE_MAX_DATA_STORE_SIZE,

    CFG_DEDUP_MIN_FRAGMENT_SIZE,
    CFG_DEDUP_MAX_FRAGMENT_SIZE,

    CFG_DEDUP_WORKER_MIN_DATA_SIZE,
    CFG_DEDUP_WORKER_THREAD_COUNT,

    CFG_DIR_WORKER_THREAD_COUNT,

    CFG_DIR_MIN_FILE_SIZE,
    CFG_DIR_MAX_FILE_SIZE,
    CFG_DIR_MAX_STORAGE_SIZE,
    CFG_DIR_MAX_CHUNK_SIZE,

    CFG_ENTRYPOINT_DEDUP_SERVICE_CONNECTION_COUNT,
    CFG_ENTRYPOINT_DIR_SERVICE_CONNECTION_COUNT,
    CFG_ENTRYPOINT_WORKER_THREAD_COUNT,
};

const std::map<std::string, uh::cluster::role> role_by_abbreviation = {
    {"storage", uh::cluster::STORAGE_SERVICE},
    {"deduplicator", uh::cluster::DEDUPLICATOR_SERVICE},
    {"directory", uh::cluster::DIRECTORY_SERVICE},
    {"entrypoint", uh::cluster::ENTRYPOINT_SERVICE}};

static constexpr const char* ENV_CFG_ENDPOINT_HOST = "UH_POD_IP";
static constexpr const char* ENV_CFG_LOG_LEVEL = "UH_LOG_LEVEL";

uh::cluster::role get_service_role(const std::string& service_role_str);

const std::string& get_service_string(const uh::cluster::role& service_role);

const std::string&
get_config_string(const uh::cluster::config_parameter& cfg_param);

} // end namespace uh::cluster

#endif // CORE_COMMON_H
