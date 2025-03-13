#include <common/telemetry/trace/trace.h>

#include <opentelemetry/exporters/ostream/span_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h>
#include <opentelemetry/sdk/trace/batch_span_processor_factory.h>
#include <opentelemetry/sdk/trace/batch_span_processor_options.h>
#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>
#include <opentelemetry/trace/provider.h>

namespace trace_api = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace otlp = opentelemetry::exporter::otlp;

namespace {
void init_propagation() {
    opentelemetry::context::propagation::GlobalTextMapPropagator::
        SetGlobalPropagator(
            opentelemetry::nostd::shared_ptr<
                opentelemetry::context::propagation::TextMapPropagator>(
                new opentelemetry::trace::propagation::HttpTraceContext()));

    auto prop = opentelemetry::context::propagation::GlobalTextMapPropagator::
        GetGlobalPropagator();
    if (!prop)
        throw std::runtime_error("Failed to set global propagator");
}
} // namespace

namespace uh::cluster {

void initialize_trace(const std::string& endpoint) {
    if (endpoint == TRACE_STDOUT_ENDPOINT) {
        auto exporter = opentelemetry::exporter::trace::
            OStreamSpanExporterFactory::Create();
        auto processor =
            trace_sdk::SimpleSpanProcessorFactory::Create(std::move(exporter));
        std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
            trace_sdk::TracerProviderFactory::Create(std::move(processor));
        trace_api::Provider::SetTracerProvider(provider);
    } else {
        trace_sdk::BatchSpanProcessorOptions bspOpts{};
        opentelemetry::exporter::otlp::OtlpGrpcExporterOptions opts;
        opts.endpoint = endpoint;
        // opts.use_ssl_credentials = true;
        // opts.ssl_credentials_cacert_as_string = "ssl-certificate";
        auto exporter = otlp::OtlpGrpcExporterFactory::Create(opts);
        auto processor = trace_sdk::BatchSpanProcessorFactory::Create(
            std::move(exporter), bspOpts);

        std::shared_ptr<opentelemetry::trace::TracerProvider> provider =
            trace_sdk::TracerProviderFactory::Create(std::move(processor));
        trace_api::Provider::SetTracerProvider(provider);
    }

    init_propagation();
}

} // namespace uh::cluster
