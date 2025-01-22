#pragma once

#include "common/etcd/utils.h"
#include <filesystem>

namespace uh::cluster {

std::size_t get_service_id(etcd_manager& client, const std::string& service,
                           const std::filesystem::path& data_dir);

} // namespace uh::cluster
