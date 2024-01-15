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
}