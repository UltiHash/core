#ifndef UH_CLUSTER_CONTEXT_H
#define UH_CLUSTER_CONTEXT_H

#include "opentelemetry/context/context.h"

#include <utility>

namespace uh::cluster {

struct context {

    [[nodiscard]] const auto& get_otel_context() const noexcept {
        return m_otel_ctx;
    }

    void set_otel_context(opentelemetry::context::Context context) {
        m_otel_ctx = std::move(context);
    }

private:
    opentelemetry::context::Context m_otel_ctx;
};

inline thread_local context CURRENT_CONTEXT;

} // namespace uh::cluster
#endif // UH_CLUSTER_CONTEXT_H
