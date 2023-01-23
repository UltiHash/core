#include "basic_options.h"


namespace uh::options
{

// ---------------------------------------------------------------------

basic_options::basic_options()
    : m_desc("General Options")
{
    m_desc.add_options()
        ("help,h", "print help screen")
        ("version,v", "output program version");
    m_hiddenDesc.add_options()
            ("vcsid", "output VCS revision ID");
}

// ---------------------------------------------------------------------

void basic_options::apply(options& opts)
{
    opts.add(m_desc);
    opts.add(m_hiddenDesc, visibility::hidden);
}

// ---------------------------------------------------------------------

void basic_options::evaluate(const boost::program_options::variables_map& vars)
{
    m_help = vars.count("help") != 0;
    m_version = vars.count("version") != 0;
    m_vcsid = vars.count("vcsid") != 0;
}

// ---------------------------------------------------------------------

bool basic_options::print_help() const
{
    return m_help;
}

// ---------------------------------------------------------------------

bool basic_options::print_version() const
{
    return m_version;
}

// ---------------------------------------------------------------------

bool basic_options::print_vcsid() const
{
    return m_vcsid;
}

// ---------------------------------------------------------------------

} // namespace uh::options
