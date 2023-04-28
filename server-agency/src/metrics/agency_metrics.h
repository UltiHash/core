#ifndef UH_METRICS_AGENCY_METRICS_H
#define UH_METRICS_AGENCY_METRICS_H

#include "metrics/service.h"

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

    class agency_metrics
    {
    public:
        agency_metrics(uh::metrics::service& service);

        prometheus::Gauge& uhv_id() const;
        prometheus::Gauge& integrated_size() const;

    private:
        prometheus::Family<prometheus::Gauge>& m_gauges;
        prometheus::Gauge& m_uhv_id;
        prometheus::Gauge& m_integrated_size;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::metrics

#endif
