#include "context.h"

#include "log.h"

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

context context::sub_context(const std::string& name) { return *this; }

void context::set_name(const std::string& name) {}

context::span_wrap::span_wrap(
    std::shared_ptr<opentelemetry::trace::Span> span) {}

context::span_wrap::~span_wrap() {}

context::context(std::shared_ptr<span_wrap> span) {}

} // namespace uh::cluster
