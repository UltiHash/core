#include "protocol_metrics.h"

#include <utility>


using namespace uh::protocol;

namespace uh::metrics
{

// ---------------------------------------------------------------------

protocol_metrics::protocol_metrics(uh::metrics::service& service)
    : m_counters(service.add_counter_family("uh_requests", "number of UH requests")),
      m_reqs_hello(m_counters.Add({{ "type", "hello" }})),
      m_reqs_read_block(m_counters.Add({{ "type", "read_block" }})),
      m_reqs_free_space(m_counters.Add({{ "type", "free_space" }})),
      m_reqs_quit(m_counters.Add({{ "type", "quit" }})),
      m_reqs_reset(m_counters.Add({{ "type", "reset" }})),
      m_reqs_next_chunk(m_counters.Add({{ "type", "next_chunk" }})),
      m_reqs_write_chunk(m_counters.Add({{ "type", "write_chunk" }})),
      m_reqs_write_small_block (m_counters.Add({{ "type", "write_small_block" }})),
      m_reqs_write_xsmall_blocks (m_counters.Add({{ "type", "write_xsmall_blocks" }})),
      m_reqs_write_chunks(m_counters.Add({{ "type", "write_chunks" }})),
      m_reqs_read_chunks(m_counters.Add({{ "type", "read_chunks" }})),
      m_reqs_allocate_chunk(m_counters.Add({{ "type", "allocate_chunk" }})),
      m_reqs_finalize(m_counters.Add({{ "type", "finalize" }})),
      m_reqs_client_statistics(m_counters.Add({{ "type", "client_statistics" }}))

{
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_hello() const
{
    return m_reqs_hello;
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

prometheus::Counter& protocol_metrics::reqs_reset() const
{
    return m_reqs_reset;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_next_chunk() const
{
    return m_reqs_next_chunk;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_write_chunk() const
{
    return m_reqs_write_chunk;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_write_small_block() const
{
    return m_reqs_write_small_block;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_client_statistics() const
{
    return m_reqs_client_statistics;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_write_xsmall_blocks() const
{
    return m_reqs_write_xsmall_blocks;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_write_chunks () const
{
    return m_reqs_write_chunks;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_read_chunks () const
{
    return m_reqs_read_chunks;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_allocate_chunk() const
{
    return m_reqs_allocate_chunk;
}

// ---------------------------------------------------------------------

prometheus::Counter& protocol_metrics::reqs_finalize() const
{
    return m_reqs_finalize;
}

// ---------------------------------------------------------------------

protocol_metrics_wrapper::protocol_metrics_wrapper(const protocol_metrics& metrics,
    std::unique_ptr<uh::protocol::request_interface>&& base) :
    m_metrics(metrics),
    m_base(std::move(base))
{
}

// ---------------------------------------------------------------------

server_information protocol_metrics_wrapper::on_hello(const std::string& client_version)
{
    m_metrics.reqs_hello().Increment();
    return m_base->on_hello(client_version);
}

// ---------------------------------------------------------------------

std::unique_ptr<io::device> protocol_metrics_wrapper::on_read_block(blob&& hash)
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

void protocol_metrics_wrapper::on_reset()
{
    m_metrics.reqs_reset().Increment();
    return m_base->on_reset();
}

// ---------------------------------------------------------------------

void protocol_metrics_wrapper::on_next_chunk(std::span<char> buffer)
{
    m_metrics.reqs_next_chunk().Increment();
}

// ---------------------------------------------------------------------

void protocol_metrics_wrapper::on_finalize()
{
    m_metrics.reqs_finalize().Increment();
    return m_base->on_finalize();
}

// ---------------------------------------------------------------------

void protocol_metrics_wrapper::on_write_chunk(std::span<char>)
{
    m_metrics.reqs_write_chunk().Increment();
}

// ---------------------------------------------------------------------

block_meta_data protocol_metrics_wrapper::on_write_small_block(std::span<char> buffer)
{
    m_metrics.reqs_write_small_block().Increment();
    return m_base->on_write_small_block(buffer);
}

// ---------------------------------------------------------------------

void protocol_metrics_wrapper::on_client_statistics(uh::protocol::client_statistics::request& client_stat)
{
    m_metrics.reqs_client_statistics().Increment();
    return m_base->on_client_statistics(client_stat);
}

// ---------------------------------------------------------------------

uh::protocol::write_xsmall_blocks::response protocol_metrics_wrapper::on_write_xsmall_blocks (const uh::protocol::write_xsmall_blocks::request &req)
{
    m_metrics.reqs_write_xsmall_blocks().Increment();
    return m_base->on_write_xsmall_blocks(req);
}

// ---------------------------------------------------------------------

uh::protocol::write_chunks::response protocol_metrics_wrapper::on_write_chunks (const uh::protocol::write_chunks::request &req)
{
    m_metrics.reqs_write_chunks().Increment(req.chunk_sizes.size());
    return m_base->on_write_chunks(req);
}

// ---------------------------------------------------------------------

std::unique_ptr<allocation> protocol_metrics_wrapper::on_allocate_chunk(std::size_t size)
{
    m_metrics.reqs_allocate_chunk().Increment();
    return m_base->on_allocate_chunk(size);
}

// ---------------------------------------------------------------------

uh::protocol::read_chunks::response protocol_metrics_wrapper::on_read_chunks(const read_chunks::request &req) {
    m_metrics.reqs_read_chunks().Increment(req.hashes.size() / 64);
    return m_base->on_read_chunks (req);
}

// ---------------------------------------------------------------------

} // namespace uh::metrics
