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
    prometheus::Gauge& used_space() const;
    prometheus::Gauge& alloc_space() const;

    prometheus::Gauge& comp_scheduled() const;
    prometheus::Gauge& comp_running() const;
private:
    prometheus::Family<prometheus::Gauge>& m_gauges;
    prometheus::Gauge& m_free_space;
    prometheus::Gauge& m_used_space;
    prometheus::Gauge& m_alloc_space;

    prometheus::Family<prometheus::Gauge>& m_gauge_comp;
    prometheus::Gauge& m_comp_scheduled;
    prometheus::Gauge& m_comp_running;
};

// ---------------------------------------------------------------------

} // namespace uh::dbn::metrics

#endif
