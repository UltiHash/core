#include "context.h"

#include "config.h"
#include "log.h"

#include <iostream>

#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_factory.h>
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter_options.h>

#include <opentelemetry/trace/propagation/http_trace_context.h>
#include <opentelemetry/trace/provider.h>

#include <opentelemetry/context/propagation/global_propagator.h>

#include <opentelemetry/sdk/trace/processor.h>
#include <opentelemetry/sdk/trace/simple_processor_factory.h>
#include <opentelemetry/sdk/trace/tracer_provider_factory.h>

using namespace opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;

namespace uh::cluster {

context::context(const std::string& name) {}

context::context(const std::vector<char>& buffer) {}

context context::sub_context(const std::string& name) {
    std::cout << "sub_context: " << name << std::endl;
    return *this;
}

void context::set_name(const std::string& name) {}

context::span_wrap::span_wrap(
    std::shared_ptr<opentelemetry::trace::Span> span) {}

context::span_wrap::~span_wrap() {}

context::context(std::shared_ptr<span_wrap> span) {}

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
