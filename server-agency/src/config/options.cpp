#include "options.h"


namespace uh::an::config
{

// ---------------------------------------------------------------------

options::options()
{
    m_basic.apply(*this);
    m_server.apply(*this);
    m_logging.apply(*this);
    m_metrics.apply(*this);
    m_cluster.apply(*this);
}

// ---------------------------------------------------------------------

void options::evaluate(const boost::program_options::variables_map& vars)
{
    m_basic.evaluate(vars);
    m_server.evaluate(vars);
    m_logging.evaluate(vars);
    m_metrics.evaluate(vars);
    m_cluster.evaluate(vars);
}

// ---------------------------------------------------------------------

const uh::options::basic_options& options::basic() const
{
    return m_basic;
}

// ---------------------------------------------------------------------

const uh::options::server_options& options::server() const
{
    return m_server;
}

// ---------------------------------------------------------------------

const uh::options::logging_options& options::logging() const
{
    return m_logging;
}

// ---------------------------------------------------------------------

const uh::options::metrics_options& options::metrics() const
{
    return m_metrics;
}

// ---------------------------------------------------------------------

const cluster::options& options::cluster() const
{
    return m_cluster;
}

// ---------------------------------------------------------------------

} // namespace uh::an::config
