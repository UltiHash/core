#ifndef SERVER_AGENCY_METRICS_H
#define SERVER_AGENCY_METRICS_H

#include <metrics/service.h>


namespace uh::an
{

// ---------------------------------------------------------------------

class metrics
{
public:
    metrics(uh::metrics::service& service);

    prometheus::Counter& reqs_hello() const;
    prometheus::Counter& reqs_write_chunk() const;
    prometheus::Counter& reqs_read_chunk() const;
private:
    uh::metrics::service& m_service;
    prometheus::Family<prometheus::Counter>& m_counters;
    prometheus::Counter& m_reqs_hello;
    prometheus::Counter& m_reqs_write_chunk;
    prometheus::Counter& m_reqs_read_chunk;
};

// ---------------------------------------------------------------------

} // namespace uh::an

#endif
