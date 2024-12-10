#ifndef CORE_COMMON_TELEMETRY_CONTEXT_H
#define CORE_COMMON_TELEMETRY_CONTEXT_H

#include <boost/asio.hpp>
#include <opentelemetry/sdk/metrics/meter_provider_factory.h>

#include <utility>

namespace uh::cluster {

struct context {

    const auto& get_otel_context() const { return m_otel_ctx; }

    void set_otel_context(opentelemetry::context::Context context) {
        m_otel_ctx = std::move(context);
    }

    boost::asio::ip::tcp::endpoint& peer() { return m_peer; }
    const boost::asio::ip::tcp::endpoint& peer() const { return m_peer; }

private:
    opentelemetry::context::Context m_otel_ctx;
    boost::asio::ip::tcp::endpoint m_peer;
};

inline thread_local context CURRENT_CONTEXT;

} // namespace uh::cluster

#endif
