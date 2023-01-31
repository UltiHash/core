#ifndef SERVER_DATABASE_CONFIG_OPTIONS_H
#define SERVER_DATABASE_CONFIG_OPTIONS_H

#include <options/basic_options.h>
#include <options/logging_options.h>
#include <options/metrics_options.h>
#include <options/server_options.h>
#include <storage/options.h>


namespace uh::dbn::config
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
    const storage::options& storage() const;

private:
    uh::options::basic_options m_basic;
    uh::options::server_options m_server;
    uh::options::logging_options m_logging;
    uh::options::metrics_options m_metrics;
    storage::options m_storage;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::config

#endif
