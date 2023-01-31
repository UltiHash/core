#ifndef SERVER_AGENCY_CONFIG_OPTIONS_H
#define SERVER_AGENCY_CONFIG_OPTIONS_H

#include <options/basic_options.h>
#include <options/logging_options.h>
#include <options/metrics_options.h>
#include <options/server_options.h>
#include <cluster/options.h>


namespace uh::an::config
{

// ---------------------------------------------------------------------

class options : public uh::options::options
{
public:
    options();

    virtual void evaluate(const boost::program_options::variables_map& vars) override;

    const uh::options::basic_options& basic() const;
    const uh::options::server_options& server() const;
    const uh::options::logging_options& logging() const;
    const uh::options::metrics_options& metrics() const;
    const cluster::options& cluster() const;

private:
    uh::options::basic_options m_basic;
    uh::options::server_options m_server;
    uh::options::logging_options m_logging;
    uh::options::metrics_options m_metrics;
    cluster::options m_cluster;
};

// ---------------------------------------------------------------------

} // namespace uh::an::config

#endif
