#include "options.h"


namespace uh::dbn
{

// ---------------------------------------------------------------------

options::options()
{
    m_basic.apply(*this);
    m_server.apply(*this);
    m_database.apply(*this);
    m_logging.apply(*this);

}

// ---------------------------------------------------------------------

void options::evaluate(const boost::program_options::variables_map& vars)
{
    m_basic.evaluate(vars);
    m_server.evaluate(vars);
    m_database.evaluate(vars);
    m_logging.evaluate(vars);
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

uh::options::database_options& options::database()
{
    return m_database;
}

// ---------------------------------------------------------------------

uh::options::logging_options& options::logging()
{
    return m_logging;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn
