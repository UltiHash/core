#ifndef SERVER_AGENCY_METRICS_MOD_H
#define SERVER_AGENCY_METRICS_MOD_H

#include <metrics/protocol_metrics.h>
#include <logging/logging_boost.h>
#include <memory>
#include <persistence/mod.h>
#include "client_metrics.h"

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

class mod
{
public:
    explicit mod(const uh::metrics::config& config, uh::an::persistence::mod& persistence);

    ~mod();

    uh::metrics::protocol_metrics& protocol();
    client_metrics& client();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::metrics

#endif
