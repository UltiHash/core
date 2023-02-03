#include "agency_connection.h"

using namespace boost::program_options;

namespace uh::client::option
{

// ---------------------------------------------------------------------

agency_connection::agency_connection()
        : m_desc("Connection Options")
{
    m_desc.add_options()
            ("agency-node,a", value<std::string> (), "<HOSTNAME[:PORT]> of agency node to connect to (port defaults to 8565)")
            ("pool-size,P", value<std::size_t>(), "pool size of connections to the agency node [optional]")
            ("metrics,M", "display connection statistics [optional]");
}

// ---------------------------------------------------------------------

void agency_connection::apply(uh::options::options& opts)
{
    opts.add(m_desc);
}

// ---------------------------------------------------------------------

void agency_connection::evaluate(const boost::program_options::variables_map& vars)
{
    m_metrics = vars.count("metrics") != 0;
}

// ---------------------------------------------------------------------

bool agency_connection::isMetrics() const {
    return m_metrics;
}

// ---------------------------------------------------------------------

void agency_connection::handle(const boost::program_options::variables_map& vars, client_config& config)
{
    if (vars.count("agency-node") > 0 )
    {
        auto agency_opt = vars["agency-node"].as<std::string>();
        auto colon = agency_opt.find(':');
        config.m_hostname = agency_opt.substr(0, colon);
        if (colon != std::string::npos)
        {
            config.m_port = std::stoi(agency_opt.substr(colon + 1));
        }
    }
    else
    {
        throw std::runtime_error("Agency node option missing.");
    }
    if (vars.count("pool_size") > 0 )
    {
        config.m_pool_size = vars["pool-size"].as<std::size_t>();
    }
}

// ---------------------------------------------------------------------

} // namespace uh::client
