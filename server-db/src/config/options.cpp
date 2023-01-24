#include "options.h"


namespace uh::dbn::config
{

// ---------------------------------------------------------------------

options::options()
{
    m_basic.apply(*this);
    m_server.apply(*this);
    m_logging.apply(*this);
    m_metrics.apply(*this);
    m_storage.apply(*this);
}

// ---------------------------------------------------------------------

void options::evaluate(const boost::program_options::variables_map& vars)
{
    m_basic.evaluate(vars);
    m_server.evaluate(vars);
    m_logging.evaluate(vars);
    m_metrics.evaluate(vars);
    m_storage.evaluate(vars); //TODO storage options
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

const storage::options& options::storage() const
{
    return m_storage;
}

// ---------------------------------------------------------------------

} // namespace uh::dbn::config
