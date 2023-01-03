#include "metrics_options.h"


using namespace boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

metrics_options::metrics_options()
    : m_desc("Metrics Options")
{
    m_desc.add_options()
        ("metrics-address", value<std::string>()->default_value("0.0.0.0:8080"), "listening address for metrics service")
        ("metrics-threads", value<std::size_t>()->default_value(2), "number of threads used for metrics service")
        ("metrics-path", value<std::string>()->default_value("/metrics"), "path used to reach metrics information");
}

// ---------------------------------------------------------------------

void metrics_options::apply(options& opts)
{
    opts.add(m_desc);
}

// ---------------------------------------------------------------------

void metrics_options::evaluate(const boost::program_options::variables_map& vars)
{
    m_config.address = vars["metrics-address"].as<std::string>();
    m_config.threads = vars["metrics-threads"].as<std::size_t>();
    m_config.path = vars["metrics-path"].as<std::string>();
}

// ---------------------------------------------------------------------

const metrics::config& metrics_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::options
