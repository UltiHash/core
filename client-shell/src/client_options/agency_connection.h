#ifndef CLIENT_OPTIONS_AGENCY_CONNECTION_H
#define CLIENT_OPTIONS_AGENCY_CONNECTION_H

#include <options/options.h>
#include "client_config.h"

namespace uh::client
{

// ---------------------------------------------------------------------

struct host_port
{
    std::string hostname;
    uint16_t port;
};

// ---------------------------------------------------------------------

class agency_connection : public options::options
{
public:

    // CLASS FUNTIONS
    agency_connection();

    // GETTERS
    [[nodiscard]] bool isMetrics() const;

    // LOGIC FUNCTIONS
    virtual uh::options::action evaluate(const boost::program_options::variables_map& vars) override;
    void handle(const boost::program_options::variables_map& vars, host_port& config);

    const host_port& config() const;

private:
    bool m_metrics = false;
    host_port m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif

