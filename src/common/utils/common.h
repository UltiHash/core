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
    {"storage", uh::cluster::STORAGE_SERVICE},
    {"deduplicator", uh::cluster::DEDUPLICATOR_SERVICE},
    {"directory", uh::cluster::DIRECTORY_SERVICE},
    {"entrypoint", uh::cluster::ENTRYPOINT_SERVICE}};

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
    DIRECTORY_BUCKET_EXISTS_REQ = 46,

    DIRECTORY_OBJECT_LIST_REQ = 39,
    DIRECTORY_OBJECT_LIST_RESP = 40,
    DIRECTORY_OBJECT_PUT_REQ = 28,
    DIRECTORY_OBJECT_GET_REQ = 29,
    DIRECTORY_OBJECT_GET_RESP = 30,
    DIRECTORY_OBJECT_DELETE_REQ = 45,

    SUCCESS = 32,
    FAILURE = 33,
};

constexpr const std::array<std::pair<uh::cluster::message_type, const char*>,
                           25>
    string_by_message_type = {{
        {STORAGE_READ_FRAGMENT_REQ, "storage_read_fragment_request"},
        {STORAGE_READ_ADDRESS_REQ, "storage_read_address_request"},
        {STORAGE_WRITE_REQ, "storage_write_request"},
        {STORAGE_SYNC_REQ, "storage_sync_request"},
        {STORAGE_REMOVE_FRAGMENT_REQ, "storage_remove_fragment_request"},
        {STORAGE_USED_REQ, "storage_used_request"},

        {DEDUPLICATOR_REQ, "deduplicator_request"},

        {DIRECTORY_BUCKET_LIST_REQ, "directory_bucket_list_request"},
        {DIRECTORY_BUCKET_PUT_REQ, "directory_bucket_put_request"},
        {DIRECTORY_BUCKET_DELETE_REQ, "directory_bucket_delete_request"},
        {DIRECTORY_BUCKET_EXISTS_REQ, "directory_bucket_exists_request"},
        {DIRECTORY_OBJECT_LIST_REQ, "directory_object_list_request"},
        {DIRECTORY_OBJECT_PUT_REQ, "directory_object_put_request"},
        {DIRECTORY_OBJECT_GET_REQ, "directory_object_get_request"},
        {DIRECTORY_OBJECT_DELETE_REQ, "directory_object_delete_request"},

        {SUCCESS, "generic_success"},
        {FAILURE, "generic_failure"},
    }};

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

constexpr std::array<std::pair<uh::cluster::config_parameter, const char*>, 25>
    string_by_config_parameter = {{
        {uh::cluster::CFG_SERVER_THREADS, "server_threads"},
        {uh::cluster::CFG_SERVER_BIND_ADDR, "server_bind_address"},
        {uh::cluster::CFG_SERVER_PORT, "server_port"},
        {uh::cluster::CFG_ENDPOINT_HOST, "endpoint_host"},
        {uh::cluster::CFG_ENDPOINT_PORT, "endpoint_port"},

        {uh::cluster::CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT,
         "gdv_storage_service_connection_count"},
        {uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L1,
         "gdv_read_cache_capacity_l1"},
        {uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L2,
         "gdv_read_cache_capacity_l2"},
        {uh::cluster::CFG_GDV_L1_SAMPLE_SIZE, "gdv_l1_sample_size"},
        {uh::cluster::CFG_GDV_MAX_DATA_STORE_SIZE, "gdv_max_data_store_size"},

        {uh::cluster::CFG_STORAGE_MIN_FILE_SIZE, "min_file_size"},
        {uh::cluster::CFG_STORAGE_MAX_FILE_SIZE, "max_file_size"},
        {uh::cluster::CFG_STORAGE_MAX_DATA_STORE_SIZE, "max_data_store_size"},

        {uh::cluster::CFG_DEDUP_MIN_FRAGMENT_SIZE, "min_fragment_size"},
        {uh::cluster::CFG_DEDUP_MAX_FRAGMENT_SIZE, "max_fragment_size"},
        {uh::cluster::CFG_DEDUP_WORKER_MIN_DATA_SIZE, "worker_min_data_size"},
        {uh::cluster::CFG_DEDUP_WORKER_THREAD_COUNT, "worker_thread_count"},

        {uh::cluster::CFG_DIR_MIN_FILE_SIZE, "min_file_size"},
        {uh::cluster::CFG_DIR_MAX_FILE_SIZE, "max_file_size"},
        {uh::cluster::CFG_DIR_MAX_STORAGE_SIZE, "max_storage_size"},
        {uh::cluster::CFG_DIR_MAX_CHUNK_SIZE, "max_chunk_size"},
        {uh::cluster::CFG_DIR_WORKER_THREAD_COUNT, "worker_thread_count"},

        {uh::cluster::CFG_ENTRYPOINT_DEDUP_SERVICE_CONNECTION_COUNT,
         "dedup_service_connection_count"},
        {uh::cluster::CFG_ENTRYPOINT_DIR_SERVICE_CONNECTION_COUNT,
         "dir_service_connection_count"},
        {uh::cluster::CFG_ENTRYPOINT_WORKER_THREAD_COUNT,
         "worker_thread_count"},
    }};

static constexpr const char* ENV_CFG_ENDPOINT_HOST = "UH_POD_IP";
static constexpr const char* ENV_CFG_LOG_LEVEL = "UH_LOG_LEVEL";

uh::cluster::role get_service_role(const std::string& service_role_str);
const std::unordered_set<uh::cluster::message_type>&
get_requests_served(uh::cluster::role message);

const std::string& get_service_string(const uh::cluster::role& service_role);

constexpr const char*
get_config_string(const uh::cluster::config_parameter& cfg_param) {
    for (const auto& entry : string_by_config_parameter) {
        if (entry.first == cfg_param)
            return entry.second;
    }

    throw std::invalid_argument("invalid configuration parameter");
}

constexpr const char*
get_message_string(const uh::cluster::message_type& type) {
    for (const auto& entry : string_by_message_type) {
        if (entry.first == type)
            return entry.second;
    }

    throw std::invalid_argument("invalid message type");
}

} // end namespace uh::cluster

#endif // CORE_COMMON_H
