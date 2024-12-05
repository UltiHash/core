#include "traces.h"
#include "common/utils/common.h"
#include "config.h"
#include "log.h"
#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"
#include "opentelemetry/trace/provider.h"

namespace uh::cluster {
namespace trace_sdk = opentelemetry::sdk::trace;

void initialize_traces_exporter(const std::string& endpoint) {
    if (endpoint.empty()) {
        return;
    }

    LOG_DEBUG() << "trace endpoint: " << endpoint;

    opentelemetry::exporter::otlp::OtlpGrpcExporterOptions trace_opts;
    trace_opts.endpoint = endpoint;

    auto exporter =
        opentelemetry::exporter::otlp::OtlpGrpcExporterFactory::Create(
            trace_opts);
    auto processor =
        trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
    trace::tracer_provider =
        trace_sdk::TracerProviderFactory::Create(std::move(processor));

    std::shared_ptr<opentelemetry::trace::TracerProvider> api_provider =
        trace::tracer_provider;
    opentelemetry::trace::Provider::SetTracerProvider(api_provider);

    opentelemetry::context::propagation::GlobalTextMapPropagator::
        SetGlobalPropagator(
            opentelemetry::nostd::shared_ptr<
                opentelemetry::context::propagation::TextMapPropagator>(
                new opentelemetry::trace::propagation::HttpTraceContext()));
}

std::shared_ptr<opentelemetry::trace::Tracer> get_tracer() {
    auto provider = opentelemetry::trace::Provider::GetTracerProvider();
    return provider->GetTracer(PROJECT_NAME, PROJECT_VERSION);
}

std::shared_ptr<opentelemetry::trace::Span>
trace::span(const std::string& name,
            const std::optional<opentelemetry::context::Context>& context) {
    opentelemetry::trace::StartSpanOptions opt;
    if (context) {
        opt.parent = *context;
    }
    return tracer()->StartSpan(name, opt);
}

opentelemetry::trace::Scope trace::scoped_span(
    const std::string& name,
    const std::optional<opentelemetry::context::Context>& context) {
    opentelemetry::trace::StartSpanOptions opt;
    if (context) {
        opt.parent = *context;
    }
    return {tracer()->StartSpan(name, opt)};
}

opentelemetry::context::Context
trace::deserialize_context(std::vector<char>&& buf) {
    carrier c(std::move(buf));
    auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::
        GetGlobalPropagator();
    auto empty_ctx = opentelemetry::context::Context();
    auto new_context = prop->Extract(c, empty_ctx);
    return new_context;
}

std::vector<char>
trace::serialize_context(const opentelemetry::context::Context& context) {
    carrier c;
    auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::
        GetGlobalPropagator();
    prop->Inject(c, context);
    return c.extract_buffer();
}

trace::~trace() {
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

trace::carrier::carrier(std::vector<char>&& buf) // for deserializing
    : m_buf(std::move(buf)) {
    auto [in, out] = zpp::bits::in_out(m_buf);
    while (in.position() < m_buf.size()) {
        std::string key;
        in(key).or_throw();
        std::string value;
        in(value).or_throw();
        m_kv.emplace(std::move(key), std::move(value));
    }
}

opentelemetry::nostd::string_view
trace::carrier::Get(opentelemetry::nostd::string_view key) const noexcept {
    auto it = m_kv.find(std::string(key));
    if (it != m_kv.end()) {
        return it->second;
    }
    return "";
}
void trace::carrier::Set(opentelemetry::nostd::string_view key,
                         opentelemetry::nostd::string_view value) noexcept {
    zpp::bits::out{m_buf, zpp::bits::size4b{}, zpp::bits::append{}}(key)
        .or_throw();
    zpp::bits::out{m_buf, zpp::bits::size4b{}, zpp::bits::append{}}(value)
        .or_throw();
}
std::vector<char> trace::carrier::extract_buffer() noexcept {
    return std::move(m_buf);
}
} // namespace uh::cluster
