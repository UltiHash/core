#ifndef UH_CLUSTER_TRACES_H
#define UH_CLUSTER_TRACES_H

#include "common/utils/common.h"
#include "opentelemetry/context/propagation/text_map_propagator.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/std/shared_ptr.h"
#include "opentelemetry/trace/provider.h"
#include "opentelemetry/trace/tracer_provider.h"

#define OPENTELEMETRY_DEPRECATED_SDK_FACTORY

namespace uh::cluster {

#ifdef OPENTELEMETRY_DEPRECATED_SDK_FACTORY
opentelemetry::nostd::shared_ptr<opentelemetry::trace::TracerProvider>
    tracer_provider;
#else
opentelemetry::nostd::shared_ptr<opentelemetry::sdk::trace::TracerProvider>
    tracer_provider;
#endif /* OPENTELEMETRY_DEPRECATED_SDK_FACTORY */

void initialize_traces_exporter(const std::string& endpoint);
std::shared_ptr<opentelemetry::trace::Tracer> get_tracer();

class carrier : public opentelemetry::context::propagation::TextMapCarrier {
public:
    carrier(const std::vector<char>& buf)
        : m_buf(buf) {
        auto [in, out] = zpp::bits::in_out(m_buf);
        if (!buf.empty()) {
            std::string key;
            zpp::bits::in{m_buf, zpp::bits::size4b{}}(key).or_throw();
        }
    }

    carrier() = default;

    [[nodiscard]] opentelemetry::nostd::string_view
    Get(opentelemetry::nostd::string_view /* key */) const noexcept override {
        return "";
    }

    void Set(opentelemetry::nostd::string_view key,
             opentelemetry::nostd::string_view value) noexcept override {
        zpp::bits::out{m_buf, zpp::bits::size4b{}, zpp::bits::append{}}(key)
            .or_throw();
        zpp::bits::out{m_buf, zpp::bits::size4b{}, zpp::bits::append{}}(value)
            .or_throw();
    }

    std::vector<char> m_buf;
    std::unordered_map<std::string, std::string> m_kv;
};

struct trace {

    static auto span(const std::string& name,
                     const std::shared_ptr<opentelemetry::trace::Span>&
                         parent_span = nullptr) {
        opentelemetry::trace::StartSpanOptions opt;
        if (parent_span) {
            opt.parent = parent_span->GetContext();
        }
        return tracer()->StartSpan(name, opt);
    }

    static auto scoped_span(const std::string& name,
                            const std::shared_ptr<opentelemetry::trace::Span>&
                                parent_span = nullptr) {
        opentelemetry::trace::StartSpanOptions opt;
        if (parent_span) {
            opt.parent = parent_span->GetContext();
        }
        return opentelemetry::trace::Scope(tracer()->StartSpan(name, opt));
    }

    ~trace() {
        if (tracer_provider) {
#ifdef OPENTELEMETRY_DEPRECATED_SDK_FACTORY
            dynamic_cast<opentelemetry::sdk::trace::TracerProvider*>(
                tracer_provider.get())
                ->ForceFlush();
#else
            tracer_provider->ForceFlush();
#endif /* OPENTELEMETRY_DEPRECATED_SDK_FACTORY */
        }

        tracer_provider.reset();
        std::shared_ptr<opentelemetry::trace::TracerProvider> none;
        opentelemetry::trace::Provider::SetTracerProvider(none);
    }

private:
    inline static std::shared_ptr<opentelemetry::trace::Tracer>& tracer() {
        static auto t = get_tracer();
        return t;
    }
};

} // namespace uh::cluster
#endif // UH_CLUSTER_TRACES_H
