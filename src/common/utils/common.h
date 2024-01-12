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

enum config_parameter : uint8_t  {
    CFG_SERVER_THREADS,
    CFG_SERVER_BIND_ADDR,
    CFG_SERVER_PORT,
    CFG_ENDPOINT_HOST,
    CFG_ENDPOINT_PORT,



};

static uh::cluster::config_parameter get_cfg_param (const std::string& cfg_param_str) {
    const std::map<std::string, uh::cluster::config_parameter> param_by_string = {
            {"server_threads", uh::cluster::CFG_SERVER_THREADS},
            {"server_bind_address", uh::cluster::CFG_SERVER_BIND_ADDR},
            {"server_port", uh::cluster::CFG_SERVER_PORT},
            {"endpoint_host", uh::cluster::CFG_ENDPOINT_HOST},
            {"endpoint_port", uh::cluster::CFG_ENDPOINT_PORT},
    };

    if (param_by_string.contains(cfg_param_str))
        return param_by_string.at(cfg_param_str);
    else
        throw std::invalid_argument ("Invalid configuration parameter: " + cfg_param_str);
}

    static std::string get_cfg_param_string (const uh::cluster::config_parameter& cfg_param) {
        const std::map<uh::cluster::config_parameter, std::string> string_by_param = {
                {uh::cluster::CFG_SERVER_THREADS,   "server_threads"},
                {uh::cluster::CFG_SERVER_BIND_ADDR, "server_bind_address"},
                {uh::cluster::CFG_SERVER_PORT,      "server_port"},
                {uh::cluster::CFG_ENDPOINT_HOST,    "endpoint_host"},
                {uh::cluster::CFG_ENDPOINT_PORT,    "endpoint_port"},
        };

        if (string_by_param.contains(cfg_param))
            return string_by_param.at(cfg_param);
        else
            throw std::invalid_argument ("Invalid configuration parameter: " + std::to_string(cfg_param));
    }

struct service_endpoint {
    uh::cluster::role role;
    std::size_t id;
    std::string host;
    std::uint16_t port;
};

static uh::cluster::role get_service_role (const std::string& service_role_str) {
    const std::map<std::string, uh::cluster::role> role_by_abbreviation = {
            {"storage",      uh::cluster::STORAGE_SERVICE},
            {"deduplicator", uh::cluster::DEDUPLICATOR_SERVICE},
            {"directory",    uh::cluster::DIRECTORY_SERVICE},
            {"entrypoint",   uh::cluster::ENTRYPOINT_SERVICE}
    };

    if (role_by_abbreviation.contains(service_role_str))
        return role_by_abbreviation.at(service_role_str);
    else
        throw std::invalid_argument ("Invalid role!");
}

static std::string get_service_string(const uh::cluster::role& service_role) {
    const std::map<uh::cluster::role, std::string> abbreviation_by_role = {
            {uh::cluster::STORAGE_SERVICE,      "storage"},
            {uh::cluster::DEDUPLICATOR_SERVICE, "deduplicator"},
            {uh::cluster::DIRECTORY_SERVICE,    "directory"},
            {uh::cluster::ENTRYPOINT_SERVICE,   "entrypoint"}
    };

    if(abbreviation_by_role.contains(service_role))
        return abbreviation_by_role.at(service_role);
    else
        throw std::invalid_argument ("Invalid role!");
}

} // end namespace uh::cluster


#endif //CORE_COMMON_H
