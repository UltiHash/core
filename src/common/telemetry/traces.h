#ifndef UH_CLUSTER_TRACES_H
#define UH_CLUSTER_TRACES_H

#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/std/shared_ptr.h"
#include "opentelemetry/trace/scope.h"
#include "opentelemetry/trace/tracer_provider.h"
#include <optional>
#include <unordered_map>
#include <vector>

#define OPENTELEMETRY_DEPRECATED_SDK_FACTORY

namespace uh::cluster {

void initialize_traces_exporter(const std::string& endpoint);
std::shared_ptr<opentelemetry::trace::Tracer> get_tracer();

struct trace {

    static std::shared_ptr<opentelemetry::trace::Span>
    span(const std::string& name,
         const std::optional<opentelemetry::context::Context>& context =
             std::nullopt);

    static opentelemetry::trace::Scope
    scoped_span(const std::string& name,
                const std::optional<opentelemetry::context::Context>& context =
                    std::nullopt);

    static opentelemetry::context::Context
    deserialize_context(std::vector<char>&& buf);

    static std::vector<char> serialize_context(
        const std::optional<opentelemetry::context::Context>& context);

    ~trace();

    friend void initialize_traces_exporter(const std::string& endpoint);

private:
    inline static std::shared_ptr<opentelemetry::trace::Tracer>& tracer() {
        static auto t = get_tracer();
        return t;
    }

#ifdef OPENTELEMETRY_DEPRECATED_SDK_FACTORY
    static inline opentelemetry::nostd::shared_ptr<
        opentelemetry::trace::TracerProvider>
        tracer_provider;
#else
    static inline opentelemetry::nostd::shared_ptr<
        opentelemetry::sdk::trace::TracerProvider>
        tracer_provider;
#endif /* OPENTELEMETRY_DEPRECATED_SDK_FACTORY */

    class carrier : public opentelemetry::context::propagation::TextMapCarrier {
    public:
        explicit carrier(std::vector<char>&& buf);

        carrier() = default; // for serializing

        [[nodiscard]] opentelemetry::nostd::string_view
        Get(opentelemetry::nostd::string_view key) const noexcept override;

        void Set(opentelemetry::nostd::string_view key,
                 opentelemetry::nostd::string_view value) noexcept override;

        [[nodiscard]] std::vector<char> extract_buffer() noexcept;

    private:
        std::vector<char> m_buf;
        std::unordered_map<std::string, std::string> m_kv;
    };
};

} // namespace uh::cluster
#endif // UH_CLUSTER_TRACES_H
