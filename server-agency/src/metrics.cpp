#include "metrics.h"


namespace uh::an
{

// ---------------------------------------------------------------------

metrics::metrics(uh::metrics::service& service)
    : m_service(service),
      m_counters(m_service.add_counter_family("uh_requests", "number of UH requests")),
      m_reqs_hello(m_counters.Add({{ "type", "hello" }})),
      m_reqs_write_chunk(m_counters.Add({{ "type", "write_chunk" }})),
      m_reqs_read_chunk(m_counters.Add({{ "type", "read_chunk" }}))
{
}

// ---------------------------------------------------------------------

prometheus::Counter& metrics::reqs_hello() const
{
    return m_reqs_hello;
}

// ---------------------------------------------------------------------

prometheus::Counter& metrics::reqs_write_chunk() const
{
    return m_reqs_write_chunk;
}

// ---------------------------------------------------------------------

prometheus::Counter& metrics::reqs_read_chunk() const
{
    return m_reqs_read_chunk;
}

// ---------------------------------------------------------------------

} // namespace uh::an
