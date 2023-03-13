#include "app_config.h"

#include <boost/program_options/variables_map.hpp>
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
    loader::evaluate(argc, argv);

    // handle config path
    handle_config();

    auto rv = loader::finalize();

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

    if (rv == action::exit)
    {
        return rv;
    }

}

// ---------------------------------------------------------------------

void application_config_base::handle_config()
{
    if (m_config.paths().empty())
    {
    }
    else
    {
        for (const auto &conf_file: m_config.paths()) {
            std::filesystem::path config_file_path = canonical(std::filesystem::path(conf_file));
            parse_config(config_file_path);
        }
    }
}

// ---------------------------------------------------------------------

void application_config_base::print_help()
{
    std::cout << visible() << "\n";
}

// ---------------------------------------------------------------------

} // namespace uh::options
