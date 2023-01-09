#ifndef SERVER_AGENCY_METRICS_MOD_H
#define SERVER_AGENCY_METRICS_MOD_H

#include <config/options.h>
#include <metrics/metrics.h>

#include <memory>


namespace uh::an::metrics
{

// ---------------------------------------------------------------------

class mod
{
public:
    mod(const config::options& options);

    ~mod();

    uh::an::metrics::metrics& metrics();

private:
    struct impl;
    std::unique_ptr<impl> m_impl;
};

// ---------------------------------------------------------------------

} // namespace uh::an::metrics

#endif
