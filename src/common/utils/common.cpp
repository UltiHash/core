#include "common.h"

#include <map>

namespace uh::cluster {

static const std::map<uh::cluster::role, std::string> abbreviation_by_role = {
    {uh::cluster::STORAGE_SERVICE, "storage"},
    {uh::cluster::DEDUPLICATOR_SERVICE, "deduplicator"},
    {uh::cluster::ENTRYPOINT_SERVICE, "entrypoint"},
    {uh::cluster::COORDINATOR_SERVICE, "coordinator"}};

const std::string& get_service_string(const role& service_role) {
    if (auto search = abbreviation_by_role.find(service_role);
        search != abbreviation_by_role.end())
        return search->second;
    else
        throw std::invalid_argument("invalid role");
}

} // namespace uh::cluster
