#ifndef SERVER_AGENCY_METRICS_MOD_H
#define SERVER_AGENCY_METRICS_MOD_H

#include <metrics/protocol_metrics.h>

#include <memory>


namespace uh::an::metrics
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const uh::metrics::config& config);

    ~mod();

    uh::metrics::protocol_metrics& protocol();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::metrics

#endif
