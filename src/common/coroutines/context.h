#ifndef UH_CLUSTER_CONTEXT_H
#define UH_CLUSTER_CONTEXT_H

#include "opentelemetry/context/context.h"
#include <boost/asio.hpp>

#include <utility>

namespace uh::cluster {

struct context {

    [[nodiscard]] const auto& get_otel_context() const noexcept {
        return m_otel_ctx;
    }

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
#endif // UH_CLUSTER_CONTEXT_H
