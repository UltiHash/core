
#ifndef UH_CLUSTER_CONTEXT_H
#define UH_CLUSTER_CONTEXT_H
#include "opentelemetry/context/context.h"
#include <atomic>
#include <optional>
#include <utility>
namespace uh::cluster {

struct context {

    [[nodiscard]]
    auto get_otel_context() const noexcept {
        return m_otel_ctx;
    }

    void set_otel_context(opentelemetry::context::Context context) {
        m_otel_ctx = std::make_optional<opentelemetry::context::Context>(std::move(context));
    }

    void init_otel_context() {
        m_otel_ctx = std::make_optional<opentelemetry::context::Context>(opentelemetry::context::Context());
    }

private:
    std::optional<opentelemetry::context::Context> m_otel_ctx;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_CONTEXT_H
