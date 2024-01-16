//
// Created by massi on 1/11/24.
//
#include "common.h"

namespace uh::cluster {
std::string get_service_string(const role &service_role) {
    const std::map<uh::cluster::role, std::string> abbreviation_by_role = {
            {uh::cluster::STORAGE_SERVICE,      "storage"},
            {uh::cluster::DEDUPLICATOR_SERVICE, "deduplicator"},
            {uh::cluster::DIRECTORY_SERVICE,    "directory"},
            {uh::cluster::ENTRYPOINT_SERVICE,   "entrypoint"}
    };

    if (abbreviation_by_role.contains(service_role))
        return abbreviation_by_role.at(service_role);
    else
        throw std::invalid_argument("Invalid role!");
}

uh::cluster::role get_service_role(const std::string &service_role_str) {
    const std::map<std::string, uh::cluster::role> role_by_abbreviation = {
            {"storage", uh::cluster::STORAGE_SERVICE},
            {"deduplicator", uh::cluster::DEDUPLICATOR_SERVICE},
            {"directory", uh::cluster::DIRECTORY_SERVICE},
            {"entrypoint", uh::cluster::ENTRYPOINT_SERVICE}
    };

    if (role_by_abbreviation.contains(service_role_str))
        return role_by_abbreviation.at(service_role_str);
    else
        throw std::invalid_argument ("Invalid role!");
}

uh::cluster::config_parameter get_config_param (const std::string& cfg_param_str) {
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

std::string get_config_string (const uh::cluster::config_parameter& cfg_param) {
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

}