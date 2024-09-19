#ifndef CORE_COMMON_UTILS_MISC_H
#define CORE_COMMON_UTILS_MISC_H

#include <filesystem>
#include <string>

namespace uh::cluster
{

std::string read_file(const std::filesystem::path& p);

} // namespace uh::cluster

#endif
