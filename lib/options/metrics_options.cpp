#include "metrics_options.h"


using namespace boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

metrics_options::metrics_options()
    : options("Metrics Options")
{
    visible().add_options()
        ("metrics-address", value<std::string>(&m_config.address)->default_value("0.0.0.0:8080"), "listening address for metrics service")
        ("metrics-threads", value<std::size_t>(&m_config.threads)->default_value(2), "number of threads used for metrics service")
        ("metrics-path", value<std::string>(&m_config.path)->default_value("/metrics"), "path used to reach metrics information");
}

// ---------------------------------------------------------------------

const metrics::config& metrics_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::options
