#include <filesystem>
#include <unistd.h>
#include "options.h"

using namespace boost::program_options;

namespace uh::an::persistence
{

// ---------------------------------------------------------------------

options::options(): uh::options::options("Persistence Options")
{
    visible().add_options()
            ("persistence-path,P", value<std::string>()->default_value("/var/lib/agency-node"), "Path where data can be stored permanently");

}

// ---------------------------------------------------------------------

uh::options::action options::evaluate(const boost::program_options::variables_map& vars)
{
    m_config.persistence_path = std::filesystem::path(vars["persistence-path"].as<std::string>());

    if (!std::filesystem::exists(m_config.persistence_path))
        throw std::runtime_error("Path doesn't exists: " + m_config.persistence_path);

    if (std::filesystem::is_directory(m_config.persistence_path))
    {
        if (access(m_config.persistence_path.c_str(), W_OK) != 0)
            throw std::runtime_error("User doesn't have write permission on: " + m_config.persistence_path);
    }
    else
        throw std::runtime_error("Path '" + m_config.persistence_path + "' is not a directory.");

    return uh::options::action::proceed;

}

// ---------------------------------------------------------------------

const persistence_config& options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
