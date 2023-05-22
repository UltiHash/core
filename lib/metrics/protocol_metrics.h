#ifndef UH_METRICS_PROTOCOL_METRICS_H
#define UH_METRICS_PROTOCOL_METRICS_H

#include <metrics/service.h>
#include <protocol/request_interface.h>

#include <memory>

#include "protocol/messages.h"

namespace uh::metrics
{

// ---------------------------------------------------------------------

class protocol_metrics
{
public:
    protocol_metrics(uh::metrics::service& service);

    prometheus::Counter& reqs_hello() const;
    prometheus::Counter& reqs_read_block() const;
    prometheus::Counter& reqs_free_space() const;
    prometheus::Counter& reqs_quit() const;
    prometheus::Counter& reqs_reset() const;
    prometheus::Counter& reqs_next_chunk() const;
    prometheus::Counter& reqs_write_chunk() const;
    prometheus::Counter& reqs_client_statistics() const;
    prometheus::Counter& reqs_write_chunks() const;
    prometheus::Counter& reqs_read_chunks() const;
    prometheus::Counter& reqs_allocate_chunk() const;
    prometheus::Counter& reqs_finalize() const;
private:
    prometheus::Family<prometheus::Counter>& m_counters;
    prometheus::Counter& m_reqs_hello;
    prometheus::Counter& m_reqs_read_block;
    prometheus::Counter& m_reqs_free_space;
    prometheus::Counter& m_reqs_quit;
    prometheus::Counter& m_reqs_reset;
    prometheus::Counter& m_reqs_next_chunk;
    prometheus::Counter& m_reqs_write_chunk;
    prometheus::Counter& m_reqs_write_chunks;
    prometheus::Counter& m_reqs_read_chunks;
    prometheus::Counter& m_reqs_allocate_chunk;
    prometheus::Counter& m_reqs_finalize;
    prometheus::Counter& m_reqs_client_statistics;
};

// ---------------------------------------------------------------------

class protocol_metrics_wrapper : public uh::protocol::request_interface
{
public:
    protocol_metrics_wrapper(const protocol_metrics& metrics,
        std::unique_ptr<uh::protocol::request_interface>&& base);

    virtual uh::protocol::server_information on_hello(const std::string& client_version) override;
    virtual std::unique_ptr<io::device> on_read_block(uh::protocol::blob&& hash) override;
    virtual std::size_t on_free_space() override;
    virtual void on_quit(const std::string& reason) override;
    virtual void on_reset() override;
    virtual void on_next_chunk(std::span<char> buffer) override;
    virtual void on_finalize() override;
    virtual void on_write_chunk(std::span<char> buffer) override;
    uh::protocol::write_chunks::response on_write_chunks (const uh::protocol::write_chunks::request &) override;
    uh::protocol::read_chunks::response on_read_chunks (const uh::protocol::read_chunks::request &) override;

    virtual std::unique_ptr<uh::protocol::allocation>
        on_allocate_chunk(std::size_t size) override;
    virtual void on_client_statistics(uh::protocol::client_statistics::request& client_stat) override;

private:
    const protocol_metrics& m_metrics;
    std::unique_ptr<uh::protocol::request_interface> m_base;
};

// ---------------------------------------------------------------------

} // namespace uh::metrics

#endif
