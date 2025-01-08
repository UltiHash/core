#include "context.h"

#include "config.h"
#include "log.h"

using namespace opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;

namespace uh::cluster {

namespace {

std::shared_ptr<context::span_wrap> deserialize(std::span<const char> buffer) {
    std::span<const uint8_t> uc_buf(
        reinterpret_cast<const uint8_t*>(&buffer[0]), buffer.size());
    std::size_t pos = 0ull;

    std::span<const uint8_t, TraceId::kSize> trace_buf(&uc_buf[pos],
                                                       TraceId::kSize);
    auto trace_id = TraceId(trace_buf);
    pos += TraceId::kSize;

    std::span<const uint8_t, SpanId::kSize> span_buf(&uc_buf[pos],
                                                     SpanId::kSize);
    auto span_id = SpanId(span_buf);
    pos += SpanId::kSize;

    auto flags = uc_buf[pos];
    pos += 1;

    auto remote = uc_buf[pos];
    pos += 1;

    auto ctx = SpanContext(trace_id, span_id, TraceFlags(flags), remote);

    return std::make_shared<context::span_wrap>(
        std::make_shared<DefaultSpan>(std::move(ctx)));
}

std::shared_ptr<Tracer> get_tracer() {
    auto provider = Provider::GetTracerProvider();
    return provider->GetTracer(PROJECT_NAME, PROJECT_VERSION);
}

} // namespace

context::context(const std::string& name)
    : m_span(std::make_shared<span_wrap>(
          get_tracer()->StartSpan(name, StartSpanOptions{}))) {}

context::context(const std::vector<char>& buffer)
    : m_span(deserialize(buffer)) {}

context context::sub_context(const std::string& name) {
    if (!m_span) {
        return context();
    }

    context rv(std::make_shared<span_wrap>(
        get_tracer()->StartSpan(name, {.parent = m_span->span->GetContext()})));

    rv.peer() = peer();
    return rv;
}

void context::set_name(const std::string& name) {
    if (m_span) {
        m_span->span->UpdateName(name);
    }
}

std::vector<char> context::serialize() const {
    std::vector<char> rv(SERIALIZED_SIZE);
    if (!m_span) {
        return rv;
    }

    auto ctx = m_span->span->GetContext();
    std::size_t pos = 0ull;

    auto trace_id = ctx.trace_id().Id();
    std::memcpy(&rv[pos], trace_id.data(), trace_id.size());
    pos += trace_id.size();

    auto span_id = ctx.span_id().Id();
    std::memcpy(&rv[pos], span_id.data(), span_id.size());
    pos += span_id.size();

    rv[pos] = ctx.trace_flags().flags();
    pos += 1;

    rv[pos] = ctx.IsRemote();
    pos += 1;

    return rv;
}

context::span_wrap::span_wrap(std::shared_ptr<opentelemetry::trace::Span> span)
    : span(std::move(span)) {}

context::span_wrap::~span_wrap() { span->End(); }

context::context(std::shared_ptr<span_wrap> span)
    : m_span(span) {}

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

    std::shared_ptr<opentelemetry::trace::TracerProvider> api_provider =
        trace_sdk::TracerProviderFactory::Create(std::move(processor));

    opentelemetry::trace::Provider::SetTracerProvider(api_provider);

    opentelemetry::context::propagation::GlobalTextMapPropagator::
        SetGlobalPropagator(
            opentelemetry::nostd::shared_ptr<
                opentelemetry::context::propagation::TextMapPropagator>(
                new opentelemetry::trace::propagation::HttpTraceContext()));
}

} // namespace uh::cluster
