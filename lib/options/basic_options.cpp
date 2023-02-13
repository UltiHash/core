#include "basic_options.h"


using namespace boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

basic_options::basic_options()
    : options("General Options")
{
    visible().add_options()
        ("help,h", value<bool>(&m_config.help)->implicit_value(true), "print help screen")
        ("version,v", value<bool>(&m_config.version)->implicit_value(true), "output program version");
    hidden().add_options()
        ("vcsid", value<bool>(&m_config.vcsid)->implicit_value(true), "output VCS revision ID");
}

// ---------------------------------------------------------------------

action basic_options::evaluate(const boost::program_options::variables_map&)
{
    return (m_config.help || m_config.version || m_config.vcsid) ? action::exit : action::proceed;
}

// ---------------------------------------------------------------------

const uh::options::config& basic_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::options
