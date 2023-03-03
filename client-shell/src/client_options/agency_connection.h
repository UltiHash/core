#ifndef CLIENT_OPTIONS_AGENCY_CONNECTION_H
#define CLIENT_OPTIONS_AGENCY_CONNECTION_H

#include <options/options.h>
#include "client_config.h"

namespace uh::client::option
{

// ---------------------------------------------------------------------

struct host_port
{
    std::uint16_t port;
    std::uint16_t pool_size;
    std::string hostname;
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
    uh::options::action evaluate(const boost::program_options::variables_map& vars) override;
    static void handle(const boost::program_options::variables_map& vars, host_port& config);

    [[nodiscard]] const host_port& config() const;

private:
    bool m_metrics = false;
    host_port m_config{0x5548,1};
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif

