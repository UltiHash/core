#include "server_options.h"


using namespace boost::program_options;

namespace uh::options
{

// ---------------------------------------------------------------------

server_options::server_options()
    : m_desc("Server Options")
{
    m_desc.add_options()
        ("port", value<uint16_t>()->default_value(net::server_config::DEFAULT_PORT), "start the server listening on this port")
        ("tls-chain", value<std::string>(), "certificate chain to use for TLS connections")
        ("tls-key", value<std::string>(), "private key to use for TLS connections")
        ("threads", value<std::size_t>()->default_value(net::server_config::DEFAULT_THREADS), "number of threads handling connections");
}

// ---------------------------------------------------------------------

void server_options::apply(options& opts)
{
    opts.add(m_desc);
}

// ---------------------------------------------------------------------

void server_options::evaluate(const boost::program_options::variables_map& vars)
{
    m_config.port = vars["port"].as<uint16_t>();
    m_config.threads = vars["threads"].as<std::size_t>();
    if (vars.count("tls-chain") > 0)
    {
        m_config.tls_chain = vars["tls-chain"].as<std::string>();
    }

    if (vars.count("tls-key") > 0)
    {
        m_config.tls_pkey = vars["tls-key"].as<std::string>();
    }
}

// ---------------------------------------------------------------------

const net::server_config& server_options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::options
