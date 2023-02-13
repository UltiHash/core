#include "options.h"


using namespace boost::program_options;

namespace uh::an::cluster
{

// ---------------------------------------------------------------------

options::options()
    : uh::options::options("Cluster Options")
{
    visible().add_options()
        ("db-node,D", value< std::vector<std::string> > (), "<HOSTNAME[:PORT]> of database node to connect to (port defaults to 21833)")
        ("node-threads", value<std::size_t>()->default_value(3), "number of threads per node");
}

// ---------------------------------------------------------------------

uh::options::action options::evaluate(const boost::program_options::variables_map& vars)
{
    cluster::config c;

    auto node_threads = vars["node-threads"].as<std::size_t>();

    if (vars.count("db-node") > 0)
    {
        for (const auto& db_opt : vars["db-node"].as< std::vector<std::string> >())
        {
            config::node_config cfg{ .pool_size = node_threads };

            auto colon = db_opt.find(':');
            if (colon != std::string::npos)
            {
                cfg.port = std::stoi(db_opt.substr(colon + 1));
                cfg.hostname = db_opt.substr(0, colon);
            }
            else
            {
                cfg.port = 21833;
            }

            c.nodes.push_back(cfg);
        }
    }
    else
    {
        throw std::runtime_error("no downstream DB nodes were configured");
    }

    std::swap(m_config, c);
    return uh::options::action::proceed;
}

// ---------------------------------------------------------------------

const config& options::config() const
{
    return m_config;
}

// ---------------------------------------------------------------------

} // namespace uh::an::cluster
