//
// Created by massi on 1/11/24.
//
#include "common.h"

namespace uh::cluster {

static const std::map<uh::cluster::role, std::string> abbreviation_by_role = {
        {uh::cluster::STORAGE_SERVICE,      "storage"},
        {uh::cluster::DEDUPLICATOR_SERVICE, "deduplicator"},
        {uh::cluster::DIRECTORY_SERVICE,    "directory"},
        {uh::cluster::ENTRYPOINT_SERVICE,   "entrypoint"}
};

static const std::map<std::string, uh::cluster::role> role_by_abbreviation = {
        {"storage", uh::cluster::STORAGE_SERVICE},
        {"deduplicator", uh::cluster::DEDUPLICATOR_SERVICE},
        {"directory", uh::cluster::DIRECTORY_SERVICE},
        {"entrypoint", uh::cluster::ENTRYPOINT_SERVICE}
};

static const std::map<std::string, uh::cluster::config_parameter> param_by_string = {
        {"server_threads", uh::cluster::CFG_SERVER_THREADS},
        {"server_bind_address", uh::cluster::CFG_SERVER_BIND_ADDR},
        {"server_port", uh::cluster::CFG_SERVER_PORT},
        {"endpoint_host", uh::cluster::CFG_ENDPOINT_HOST},
        {"endpoint_port", uh::cluster::CFG_ENDPOINT_PORT},
};

static const std::map<uh::cluster::config_parameter, std::string> string_by_param = {
        {uh::cluster::CFG_SERVER_THREADS,   "server_threads"},
        {uh::cluster::CFG_SERVER_BIND_ADDR, "server_bind_address"},
        {uh::cluster::CFG_SERVER_PORT,      "server_port"},
        {uh::cluster::CFG_ENDPOINT_HOST,    "endpoint_host"},
        {uh::cluster::CFG_ENDPOINT_PORT,    "endpoint_port"},
};



const std::string& get_service_string(const role &service_role) {
    if (auto search = abbreviation_by_role.find(service_role); search != abbreviation_by_role.end())
        return search->second;
    else
        throw std::invalid_argument("Invalid role!");
}

uh::cluster::role get_service_role(const std::string &service_role_str) {
    if (auto search = role_by_abbreviation.find(service_role_str); search != role_by_abbreviation.end())
        return search->second;
    else
        throw std::invalid_argument ("Invalid role!");
}

uh::cluster::config_parameter get_config_param (const std::string& cfg_param_str) {
    if (auto search = param_by_string.find(cfg_param_str); search != param_by_string.end())
        return search->second;
    else
        throw std::invalid_argument ("Invalid configuration parameter: " + cfg_param_str);
}

const std::string& get_config_string (const uh::cluster::config_parameter& cfg_param) {
    if (auto search = string_by_param.find(cfg_param); search != string_by_param.end())
        return search->second;
    else
        throw std::invalid_argument ("Invalid configuration parameter: " + std::to_string(cfg_param));
}

}