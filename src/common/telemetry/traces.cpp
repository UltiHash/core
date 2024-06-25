#include "traces.h"
#include "opentelemetry/context/propagation/global_propagator.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h"
#include "opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h"
#include "opentelemetry/sdk/trace/processor.h"
#include "opentelemetry/sdk/trace/simple_processor_factory.h"
#include "opentelemetry/sdk/trace/tracer_provider_factory.h"
#include "opentelemetry/trace/propagation/http_trace_context.h"

namespace uh::cluster {
namespace trace_sdk = opentelemetry::sdk::trace;

void initialize_traces_exporter(const std::string& endpoint) {
    opentelemetry::exporter::otlp::OtlpGrpcExporterOptions trace_opts;
    trace_opts.endpoint = endpoint;
    auto exporter =
        opentelemetry::exporter::otlp::OtlpGrpcExporterFactory::Create(
            trace_opts);
    auto processor =
        trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
    tracer_provider =
        trace_sdk::TracerProviderFactory::Create(std::move(processor));

    std::shared_ptr<opentelemetry::trace::TracerProvider> api_provider =
        tracer_provider;
    opentelemetry::trace::Provider::SetTracerProvider(api_provider);

    opentelemetry::context::propagation::GlobalTextMapPropagator::
        SetGlobalPropagator(
            opentelemetry::nostd::shared_ptr<
                opentelemetry::context::propagation::TextMapPropagator>(
                new opentelemetry::trace::propagation::HttpTraceContext()));
}

std::shared_ptr<opentelemetry::trace::Tracer> get_tracer() {
    auto provider = opentelemetry::trace::Provider::GetTracerProvider();
    return provider->GetTracer("uh-cluster-traces", OPENTELEMETRY_SDK_VERSION);
}
} // namespace uh::cluster
