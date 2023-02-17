#ifndef UTIL_SYSTEM_H
#define UTIL_SYSTEM_H

#include <filesystem>
#include <string>


namespace uh::util
{

// ---------------------------------------------------------------------

class system
{
public:
    static std::filesystem::path config_dir(const std::string& project);
};

// ---------------------------------------------------------------------

} // namespace uh::util

#endif
