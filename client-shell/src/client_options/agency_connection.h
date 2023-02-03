#ifndef CLIENT_OPTIONS_AGENCY_CONNECTION_H
#define CLIENT_OPTIONS_AGENCY_CONNECTION_H

#include <options/options.h>
#include "client_config.h"

namespace uh::client::option
{

// ---------------------------------------------------------------------

class agency_connection
{
public:

    // CLASS FUNTIONS
    agency_connection();

    // GETTERS
    [[nodiscard]] bool isMetrics() const;

    // LOGIC FUNCTIONS
    void apply(uh::options::options& opts);
    void evaluate(const boost::program_options::variables_map& vars);
    void handle(const boost::program_options::variables_map& vars, client_config& config);

private:
    boost::program_options::options_description m_desc;
    bool m_metrics = false;
};

// ---------------------------------------------------------------------

} // namespace uh::client

#endif

