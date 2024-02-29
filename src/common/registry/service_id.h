#ifndef COMMON_REGISTRY_SERVICE_ID_H
#define COMMON_REGISTRY_SERVICE_ID_H

#include <etcd/SyncClient.hpp>
#include <filesystem>
#include <string>

namespace uh::cluster {

std::size_t get_service_id(etcd::SyncClient& client, const std::string& service,
                           const std::filesystem::path& data_dir);

} // namespace uh::cluster

#endif
