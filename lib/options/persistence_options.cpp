#include "options/persistence_options.h"

using namespace boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

persistence_options::persistence_options(): uh::options::options("Persistence Options")
{
    visible().add_options()
            ("persistence-path,P", value<std::string>()->default_value("/var/lib"), "Path where important server information can be stored permanently");
}

// ---------------------------------------------------------------------

const persistence_config& persistence_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::persistence
