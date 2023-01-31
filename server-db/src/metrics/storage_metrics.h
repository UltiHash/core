#ifndef UH_METRICS_storage_METRICS_H
#define UH_METRICS_storage_METRICS_H

#include <metrics/service.h>

namespace uh::dbn::metrics
{

// ---------------------------------------------------------------------

class storage_metrics
{
public:
    storage_metrics(uh::metrics::service& service);

    prometheus::Gauge& free_space() const;
private:
    prometheus::Family<prometheus::Gauge>& m_gauges;
    prometheus::Gauge& m_free_space;
};

// ---------------------------------------------------------------------

} // namespace uh::metrics

#endif
