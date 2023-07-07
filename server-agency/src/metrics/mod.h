#ifndef SERVER_AGENCY_METRICS_MOD_H
#define SERVER_AGENCY_METRICS_MOD_H

#include <metrics/protocol_metrics.h>
#include <memory>
#include <state/mod.h>
#include "client_metrics.h"

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const uh::metrics::config& config, uh::an::state::mod& persistence);

    ~mod();

    uh::metrics::protocol_metrics& protocol();
    client_metrics_state& client();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::metrics

#endif
