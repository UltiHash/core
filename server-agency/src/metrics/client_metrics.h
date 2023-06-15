#ifndef UH_METRICS_AGENCY_METRICS_H
#define UH_METRICS_AGENCY_METRICS_H

#include <protocol/messages.h>
#include <metrics/service.h>
#include <state/client_metrics_state.h>

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

    class client_metrics_state
    {
    public:
        client_metrics_state(uh::metrics::service& service,
                             uh::an::state::client_metrics_state& persisted_client_metrics);

        void set_uhv_metrics(const uh::protocol::client_statistics::request& client_stat) const;

    private:
        prometheus::Family<prometheus::Gauge>& m_gauges;
        uh::an::state::client_metrics_state& m_persisted_client_metrics;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::metrics

#endif
