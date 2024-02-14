#include "common.h"

namespace uh::cluster {

static const std::map<uh::cluster::role, std::string> abbreviation_by_role = {
    {uh::cluster::STORAGE_SERVICE, "storage"},
    {uh::cluster::DEDUPLICATOR_SERVICE, "deduplicator"},
    {uh::cluster::DIRECTORY_SERVICE, "directory"},
    {uh::cluster::ENTRYPOINT_SERVICE, "entrypoint"}};

const std::map<uh::cluster::role, std::unordered_set<uh::cluster::message_type>>
    requests_served_by_role = {
        {STORAGE_SERVICE,
         {STORAGE_READ_FRAGMENT_REQ, STORAGE_READ_ADDRESS_REQ,
          STORAGE_WRITE_REQ, STORAGE_SYNC_REQ, STORAGE_REMOVE_FRAGMENT_REQ,
          STORAGE_USED_REQ}},
        {DEDUPLICATOR_SERVICE, {DEDUPLICATOR_REQ}},
        {DIRECTORY_SERVICE,
         {DIRECTORY_BUCKET_LIST_REQ, DIRECTORY_BUCKET_PUT_REQ,
          DIRECTORY_BUCKET_DELETE_REQ, DIRECTORY_BUCKET_EXISTS_REQ,
          DIRECTORY_OBJECT_LIST_REQ, DIRECTORY_OBJECT_PUT_REQ,
          DIRECTORY_OBJECT_GET_REQ, DIRECTORY_OBJECT_DELETE_REQ}}};

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

const std::unordered_set<uh::cluster::message_type>&
get_requests_served(uh::cluster::role role) {
    if (auto search = requests_served_by_role.find(role);
        search != requests_served_by_role.end())
        return search->second;

    throw std::invalid_argument("invalid role");
}

} // namespace uh::cluster