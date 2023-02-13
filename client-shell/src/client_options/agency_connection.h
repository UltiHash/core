#ifndef CLIENT_OPTIONS_AGENCY_CONNECTION_H
#define CLIENT_OPTIONS_AGENCY_CONNECTION_H

#include <options/options.h>
#include "client_config.h"

namespace uh::client
{

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
    void handle(const boost::program_options::variables_map& vars, client_config& config);

    const client_config& config() const;

private:
    bool m_metrics = false;
    client_config m_config;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif

