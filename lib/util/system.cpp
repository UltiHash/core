#include "system.h"


namespace uh::util
{

// ---------------------------------------------------------------------

std::filesystem::path system::config_dir(const std::string& project)
{
    return std::filesystem::path("/etc") / project;
}

// ---------------------------------------------------------------------

} // namespace uh::util
