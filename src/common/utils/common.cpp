//
// Created by massi on 1/11/24.
//
#include "common.h"

namespace uh::cluster {

static const std::map<uh::cluster::config_parameter, std::string> string_by_param = {
        {uh::cluster::CFG_SERVER_THREADS,   "server_threads"},
        {uh::cluster::CFG_SERVER_BIND_ADDR, "server_bind_address"},
        {uh::cluster::CFG_SERVER_PORT,      "server_port"},
        {uh::cluster::CFG_ENDPOINT_HOST,    "endpoint_host"},
        {uh::cluster::CFG_ENDPOINT_PORT,    "endpoint_port"},

        {uh::cluster::CFG_GDV_STORAGE_SERVICE_CONNECTION_COUNT, "gdv_storage_service_connection_count"},
        {uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L1,           "gdv_read_cache_capacity_l1"},
        {uh::cluster::CFG_GDV_READ_CACHE_CAPACITY_L2,           "gdv_read_cache_capacity_l2"},
        {uh::cluster::CFG_GDV_L1_SAMPLE_SIZE,                   "gdv_l1_sample_size"},
        {uh::cluster::CFG_GDV_MAX_DATA_STORE_SIZE,              "gdv_max_data_store_size"},

        {uh::cluster::CFG_STORAGE_MIN_FILE_SIZE,        "min_file_size"},
        {uh::cluster::CFG_STORAGE_MAX_FILE_SIZE,        "max_file_size"},
        {uh::cluster::CFG_STORAGE_MAX_DATA_STORE_SIZE,  "max_data_store_size"},

        {uh::cluster::CFG_DEDUP_MIN_FRAGMENT_SIZE,      "min_fragment_size"},
        {uh::cluster::CFG_DEDUP_MAX_FRAGMENT_SIZE,      "max_fragment_size"},
        {uh::cluster::CFG_DEDUP_WORKER_MIN_DATA_SIZE,   "worker_min_data_size"},
        {uh::cluster::CFG_DEDUP_WORKER_THREAD_COUNT,    "worker_thread_count"},

        {uh::cluster::CFG_DIR_MIN_FILE_SIZE,            "min_file_size"},
        {uh::cluster::CFG_DIR_MAX_FILE_SIZE,            "max_file_size"},
        {uh::cluster::CFG_DIR_MAX_STORAGE_SIZE,         "max_storage_size"},
        {uh::cluster::CFG_DIR_MAX_CHUNK_SIZE,           "max_chunk_size"},
        {uh::cluster::CFG_DIR_WORKER_THREAD_COUNT,      "worker_thread_count"},

        {uh::cluster::CFG_ENTRYPOINT_DEDUP_SERVICE_CONNECTION_COUNT, "dedup_service_connection_count"},
        {uh::cluster::CFG_ENTRYPOINT_DIR_SERVICE_CONNECTION_COUNT,   "dir_service_connection_count"},
        {uh::cluster::CFG_ENTRYPOINT_WORKER_THREAD_COUNT,            "worker_thread_count"},
};


uh::cluster::role get_service_role(const std::string &service_role_str) {
    static const std::map<std::string, role> role_by_abbreviation = {
            {"storage", STORAGE_SERVICE},
            {"deduplicator", DEDUPLICATOR_SERVICE},
            {"directory", DIRECTORY_SERVICE},
            {"entrypoint", ENTRYPOINT_SERVICE}
    };

    if (auto search = role_by_abbreviation.find(service_role_str); search != role_by_abbreviation.end())
        return search->second;
    else
        throw std::invalid_argument ("Invalid role!");
}

const std::string& get_config_string (const uh::cluster::config_parameter& cfg_param) {
    if (auto search = string_by_param.find(cfg_param); search != string_by_param.end())
        return search->second;
    else
        throw std::invalid_argument ("Invalid configuration parameter: " + std::to_string(cfg_param));
}

}
