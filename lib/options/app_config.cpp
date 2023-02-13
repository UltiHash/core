#include "app_config.h"

#include <boost/program_options/variables_map.hpp>


namespace po = boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

application_config_base::application_config_base()
{
    add(m_basic);
}

// ---------------------------------------------------------------------

action application_config_base::evaluate(int argc, const char** argv)
{
    auto rv = loader::evaluate(argc, argv);

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

    return rv;
}

// ---------------------------------------------------------------------

void application_config_base::print_help()
{
    std::cout << visible() << "\n";
}

// ---------------------------------------------------------------------

} // namespace uh::options
