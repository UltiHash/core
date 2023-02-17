#include "app_config.h"

#include <boost/program_options/variables_map.hpp>


namespace po = boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

application_config_base::application_config_base()
{
    m_loader.add(m_basic);
    m_loader.add(m_config_file);
}

// ---------------------------------------------------------------------

action application_config_base::evaluate(int argc, const char** argv)
{
    auto rv = m_loader.parse(argc, argv);

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

    for (const auto& path : m_config_file.paths())
    {
        m_loader.parse(path);
    }

    return rv;
}

// ---------------------------------------------------------------------

void application_config_base::print_help()
{
    std::cout << m_loader.visible() << "\n";
}

// ---------------------------------------------------------------------

void application_config_base::add(options& opts)
{
    m_loader.add(opts);
}

// ---------------------------------------------------------------------

} // namespace uh::options
