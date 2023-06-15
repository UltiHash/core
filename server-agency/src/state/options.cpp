#include <filesystem>
#include "options.h"
#include <util/exception.h>

using namespace boost::program_options;

namespace uh::an::state
{

// ---------------------------------------------------------------------

enum class OptionsEnum {DataDirectory};

constexpr const char* optionString(OptionsEnum n)
{
    switch (n)
    {
        case OptionsEnum::DataDirectory: return "data-directory";
        default: THROW(util::exception, "Not implemented option");
    }
}

// ---------------------------------------------------------------------

options::options()
        : uh::options::options("Storage Options")
{
    visible().add_options()
            (optionString(OptionsEnum::DataDirectory),
             value<std::string>()->default_value(std::string(uh::an::state::storage_config::default_data_directory)),
             "Directory where the agency node stores its data");
}

// ---------------------------------------------------------------------

uh::options::action options::evaluate(const boost::program_options::variables_map& vars)
{
    storage_config c;
    c.data_directory = vars[optionString(OptionsEnum::DataDirectory)].as<std::string>();

    if (std::filesystem::path(c.data_directory).is_relative())
    {
        THROW(util::illegal_args, "The data directory must be an absolute path.");
    }

    if (std::filesystem::exists(c.data_directory))
    {
        if (!std::filesystem::is_directory(c.data_directory))
        {
            THROW(util::illegal_args, "The given path for data-directory is not a directory: " + c.data_directory.string());
        }
        if (access(c.data_directory.c_str(), W_OK | R_OK ) != 0)
        {
            THROW(util::illegal_args, "The user doesn't have read and write permission on directory: " + c.data_directory.string());
        }
    }
    else
    {
        std::filesystem::create_directory(c.data_directory);

    }

    c.an_metrics = c.data_directory / std::filesystem::path("state");
    std::filesystem::create_directory(c.an_metrics);

    std::swap(m_config, c);
    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

const storage_config& options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::an::persistence
