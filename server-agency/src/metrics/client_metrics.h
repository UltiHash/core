#ifndef UH_METRICS_AGENCY_METRICS_H
#define UH_METRICS_AGENCY_METRICS_H

#include <protocol/messages.h>
#include <metrics/service.h>
#include <persistence/storage/client_metrics_persistence.h>

namespace uh::an::metrics
{

// ---------------------------------------------------------------------

    class client_metrics
    {
    public:
        explicit client_metrics(uh::metrics::service& service,
                                const uh::an::persistence::client_metrics& persisted_client_metrics);

        void set_uhv_metrics(const uh::protocol::client_statistics::request& client_stat) const;

    private:
        prometheus::Family<prometheus::Gauge>& m_gauges;
    };

// ---------------------------------------------------------------------

} // namespace uh::an::metrics

#endif
