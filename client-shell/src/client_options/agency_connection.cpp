#include "agency_connection.h"

using namespace boost::program_options;

namespace uh::client::option
{

// ---------------------------------------------------------------------

agency_connection::agency_connection()
    : options("Connection Options")
{
    visible().add_options()
            ("agency-node,a", value<std::string>(), "<HOSTNAME[:PORT]> of agency node to connect to (port defaults to 8565)")
            ("pool-size,P", value<std::uint16_t>(&m_config.pool_size), "pool size of connections to the agency node [optional]");
}

// ---------------------------------------------------------------------

options::action agency_connection::evaluate(const boost::program_options::variables_map& vars)
{
    m_metrics = vars.count("metrics") != 0;
    handle(vars, m_config);
    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

bool agency_connection::isMetrics() const
{
    return m_metrics;
}

// ---------------------------------------------------------------------

void agency_connection::handle(const boost::program_options::variables_map& vars, host_port& config)
{
    if (vars.count("agency-node") > 0 )
    {
        auto agency_opt = vars["agency-node"].as<std::string>();
        auto colon = agency_opt.find(':');
        config.hostname = agency_opt.substr(0, colon);
        if (colon != std::string::npos)
        {
            config.port = std::stoi(agency_opt.substr(colon + 1));
        }
    }
    else
    {
        throw std::runtime_error("Agency node option missing.");
    }
}

// ---------------------------------------------------------------------

const host_port& agency_connection::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::client
