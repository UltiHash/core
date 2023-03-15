#include "app_config.h"
#include <filesystem>
#include <boost/program_options/parsers.hpp>

namespace po = boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

application_config_base::application_config_base()
{
    add(m_config);
    add(m_basic);
}

// ---------------------------------------------------------------------

action application_config_base::evaluate(int argc, const char** argv)
{
    action rv;
    try
    {
        loader::evaluate(argc, argv);
        handle_config();
        rv = loader::finalize();

        const auto& basic = m_basic.config();
        if (basic.help)
        {
            print_help();
        }

        if (basic.version)
        {
            print_version();
        }

        if (basic.vcsid)
        {
            print_vcsid();
        }
    }
    catch (const std::exception& e)
    {
        rv = action::exit;
        std::cout << "error: " << e.what() << "\n";
    }

    return rv;
}

// ---------------------------------------------------------------------

void application_config_base::handle_config()
{
    for (const auto& conf_file: m_config.paths())
    {
        std::filesystem::path config_file_path = canonical(std::filesystem::path(conf_file));
        parse(config_file_path);
    }
}

// ---------------------------------------------------------------------

void application_config_base::print_help()
{
    std::cout << visible() << "\n";
}

// ---------------------------------------------------------------------

} // namespace uh::options
