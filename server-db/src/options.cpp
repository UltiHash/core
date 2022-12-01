#include "options.h"


namespace uh::dbn
{

// ---------------------------------------------------------------------

options::options()
{
    m_basic.apply(*this);
    m_server.apply(*this);
}

// ---------------------------------------------------------------------

void options::evaluate(const boost::program_options::variables_map& vars)
{
    m_basic.evaluate(vars);
    m_server.evaluate(vars);
}

// ---------------------------------------------------------------------

uh::options::basic_options& options::basic()
{
    return m_basic;
}

// ---------------------------------------------------------------------

uh::options::server_options& options::server()
{
    return m_server;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn
