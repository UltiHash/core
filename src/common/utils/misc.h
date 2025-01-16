#pragma once

#include <filesystem>
#include <string>

namespace uh::cluster {

std::string read_file(const std::filesystem::path& p);

} // namespace uh::cluster
