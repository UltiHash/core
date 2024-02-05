//
// Created by masi on 7/19/23.
//

#ifndef CORE_COMMON_H
#define CORE_COMMON_H
#include <string>
#include <vector>
#include <map>
#include "common_types.h"

namespace uh::cluster {

enum message_type: uint8_t {
    READ_REQ = 10,
    READ_RESP = 11,
    WRITE_REQ = 12,
    WRITE_RESP = 13,
    SYNC_REQ = 20,
    SYNC_OK = 21,
    REMOVE_REQ = 22,
    REMOVE_OK = 23,
    USED_REQ = 24,
    USED_RESP = 25,
    DEDUPE_REQ = 26,
    DEDUPE_RESP = 27,
    DIR_PUT_OBJ_REQ = 28,
    DIR_GET_OBJ_REQ = 29,
    DIR_GET_OBJ_RESP = 30,
    DIR_PUT_BUCKET_REQ = 31,
    SUCCESS = 32,
    FAILURE = 33,
    STOP = 34,
    RECOVER_REQ = 35,
    RECOVER_RESP = 36,
    DIR_LIST_BUCKET_REQ = 37,
    DIR_LIST_BUCKET_RESP = 38,
    DIR_LIST_OBJ_REQ = 39,
    DIR_LIST_OBJ_RESP = 40,
    DIR_DELETE_BUCKET_REQ = 41,
    DIR_DELETE_BUCKET_RESP = 42,
    READ_ADDRESS_REQ = 43,
    READ_ADDRESS_RESP = 44,
    DIR_DELETE_OBJ_REQ = 45,
    DIR_BUCKET_EXISTS = 46,
};

enum role: uint8_t {
    STORAGE_SERVICE,
    DEDUPLICATOR_SERVICE,
    DIRECTORY_SERVICE,
    ENTRYPOINT_SERVICE,
};

enum ec_type: uint8_t {
    NONE = 0,
    XOR = 1,
};

enum config_parameter  {
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

uh::cluster::role get_service_role (const std::string& service_role_str);

constexpr std::string to_string(role r);

const std::string& get_config_string (const uh::cluster::config_parameter& cfg_param);

constexpr std::string to_string(role r) {
    switch (r) {
        case STORAGE_SERVICE: return "storage";
        case DEDUPLICATOR_SERVICE: return "deduplicator";
        case DIRECTORY_SERVICE: return "directory";
        case ENTRYPOINT_SERVICE: return "entrypoint";
    }

    throw std::invalid_argument("unsupported role type");
}

} // end namespace uh::cluster


#endif //CORE_COMMON_H
