#include "protocol_metrics.h"


using namespace uh::protocol;

namespace uh::metrics
{

// ---------------------------------------------------------------------

protocol_metrics::protocol_metrics(uh::metrics::service& service)
    : m_counters(service.add_counter_family("uh_requests", "number of UH requests")),
      m_reqs_hello(m_counters.Add({{ "type", "hello" }})),
      m_reqs_write_block(m_counters.Add({{ "type", "write_block" }})),
      m_reqs_read_block(m_counters.Add({{ "type", "read_block" }})),
      m_reqs_free_space(m_counters.Add({{ "type", "free_space" }})),
      m_reqs_quit(m_counters.Add({{ "type", "quit" }}))
{
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_hello() const
{
    return m_reqs_hello;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_write_block() const
{
    return m_reqs_write_block;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_read_block() const
{
    return m_reqs_read_block;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_free_space() const
{
    return m_reqs_free_space;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_quit() const
{
    return m_reqs_quit;
}

// ---------------------------------------------------------------------

protocol_metrics_wrapper::protocol_metrics_wrapper(
    const protocol_metrics& metrics,
    std::unique_ptr<server>&& base)
    : m_base(std::move(base)),
      m_metrics(metrics)
{
}

// ---------------------------------------------------------------------

server_information protocol_metrics_wrapper::on_hello(const std::string& client_version)
{
    m_metrics.reqs_hello().Increment();
    return m_base->on_hello(client_version);
}

// ---------------------------------------------------------------------

blob protocol_metrics_wrapper::on_write_block(blob&& data)
{
    m_metrics.reqs_write_block().Increment();
    return m_base->on_write_block(std::move(data));
}

// ---------------------------------------------------------------------

blob protocol_metrics_wrapper::on_read_block(blob&& hash)
{
    m_metrics.reqs_read_block().Increment();
    return m_base->on_read_block(std::move(hash));
}

// ---------------------------------------------------------------------

std::size_t protocol_metrics_wrapper::on_free_space()
{
    m_metrics.reqs_free_space().Increment();
    return m_base->on_free_space();
}

// ---------------------------------------------------------------------

void protocol_metrics_wrapper::on_quit(const std::string& reason)
{
    m_metrics.reqs_quit().Increment();
    return m_base->on_quit(reason);
}

// ---------------------------------------------------------------------

} // namespace uh::metrics
