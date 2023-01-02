#include "options.h"


namespace uh::an
{

// ---------------------------------------------------------------------

options::options()
{
    m_basic.apply(*this);
    m_server.apply(*this);
    m_logging.apply(*this);
    m_metrics.apply(*this);
}

// ---------------------------------------------------------------------

void options::evaluate(const boost::program_options::variables_map& vars)
{
    m_basic.evaluate(vars);
    m_server.evaluate(vars);
    m_logging.evaluate(vars);
    m_metrics.evaluate(vars);
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

uh::options::logging_options& options::logging()
{
    return m_logging;
}

// ---------------------------------------------------------------------

uh::options::metrics_options& options::metrics()
{
    return m_metrics;
}

// ---------------------------------------------------------------------

} // namespace uh::an
