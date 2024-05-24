#include "common.h"

namespace uh::cluster {

static const std::map<uh::cluster::role, std::string> abbreviation_by_role = {
    {uh::cluster::STORAGE_SERVICE, "storage"},
    {uh::cluster::DEDUPLICATOR_SERVICE, "deduplicator"},
    {uh::cluster::ENTRYPOINT_SERVICE, "entrypoint"}};

const std::string& get_service_string(const role& service_role) {
    if (auto search = abbreviation_by_role.find(service_role);
        search != abbreviation_by_role.end())
        return search->second;
    else
        throw std::invalid_argument("invalid role");
}

uh::cluster::role get_service_role(const std::string& service_role_str) {
    if (auto search = role_by_abbreviation.find(service_role_str);
        search != role_by_abbreviation.end())
        return search->second;

    throw std::invalid_argument("invalid role");
}

} // namespace uh::cluster
