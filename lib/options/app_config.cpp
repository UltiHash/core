#include "app_config.h"

#include <util/system.h>
#include <util/user.h>

#include <boost/program_options/variables_map.hpp>


namespace po = boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

application_config_base::application_config_base(const std::string& project)
    : m_project(project)
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

    if (m_config_file.paths().empty())
    {
        m_loader.try_parse(util::system::config_dir(m_project) / (m_project + ".ini"));
        m_loader.try_parse(util::user::home() / ("." + m_project + "rc"));
    }
    else
    {
        for (const auto& path : m_config_file.paths())
        {
            m_loader.parse(path);
        }
    }

    // CLI wins over configuration file as flags given via command line args are
    // directly given by the user. We parse again in order to overwrite values
    // written from the configuration file.
    return m_loader.parse(argc, argv);
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
