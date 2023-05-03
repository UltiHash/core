#ifndef UH_METRICS_AGENCY_METRICS_H
#define UH_METRICS_AGENCY_METRICS_H

#include "metrics/service.h"

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

    class client_metrics
    {
    public:
        explicit client_metrics(uh::metrics::service& service);

        void set_uhv_metrics(const std::pair<std::string, std::uint64_t>& label) const;

    private:
        prometheus::Family<prometheus::Gauge>& m_gauges;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::metrics

#endif
