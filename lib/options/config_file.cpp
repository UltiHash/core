#include "config_file.h"


using namespace boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

config_file::config_file()
    : options("Configuration file")
{
    visible().add_options()
        ("config,c", value<std::vector<std::string>>(&m_config_paths)->multitoken(), "load configuration file");
}

// ---------------------------------------------------------------------

const std::vector<std::string>& config_file::paths() const
{
    return m_config_paths;
}

// ---------------------------------------------------------------------

} // namespace uh::options
