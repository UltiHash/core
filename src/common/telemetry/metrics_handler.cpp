#include "metrics_handler.h"

#include "common/utils/log.h"
#include "opentelemetry/exporters/ostream/metric_exporter_factory.h"

namespace metric_sdk = opentelemetry::sdk::metrics;
namespace common = opentelemetry::common;
namespace metrics_api = opentelemetry::metrics;
namespace otlp_exporter = opentelemetry::exporter::otlp;

namespace uh::cluster {

constexpr metric_sdk::PeriodicExportingMetricReaderOptions ostream_options{
    .export_interval_millis = std::chrono::milliseconds(10000),
    .export_timeout_millis = std::chrono::milliseconds(500)};

constexpr metric_sdk::PeriodicExportingMetricReaderOptions otlp_options{
    .export_interval_millis = std::chrono::milliseconds(1000),
    .export_timeout_millis = std::chrono::milliseconds(500)};

metrics_handler::metrics_handler(const uh::cluster::role service_role,
                                 const std::string& telemetry_endpoint)
    : m_served_request_types(get_requests_served(service_role)) {
    initialize_metrics_exporter(telemetry_endpoint);

    m_served_request_types.insert(uh::cluster::SUCCESS);
    m_served_request_types.insert(uh::cluster::FAILURE);
    for (auto message_type : m_served_request_types) {
        create_uint_counter(get_message_string(message_type));
    }
}

void metrics_handler::initialize_metrics_exporter(
    const std::string& telemetry_endpoint) {
    if (telemetry_endpoint.empty())
        initialize_metrics_ostream_exporter();
    else
        initialize_metrics_otlp_grpc_exporter(telemetry_endpoint);
}

void metrics_handler::initialize_metrics_otlp_grpc_exporter(
    const std::string& telemetry_endpoint) {
    otlp_exporter::OtlpGrpcMetricExporterOptions exporter_options;
    exporter_options.endpoint = telemetry_endpoint;
    auto exporter =
        otlp_exporter::OtlpGrpcMetricExporterFactory::Create(exporter_options);

    auto reader = metric_sdk::PeriodicExportingMetricReaderFactory::Create(
        std::move(exporter), otlp_options);

    auto context = metric_sdk::MeterContextFactory::Create();
    context->AddMetricReader(std::move(reader));

    auto u_provider =
        metric_sdk::MeterProviderFactory::Create(std::move(context));
    std::shared_ptr<opentelemetry::metrics::MeterProvider> provider(
        std::move(u_provider));

    metrics_api::Provider::SetMeterProvider(provider);
}

void metrics_handler::initialize_metrics_ostream_exporter() {
    auto exporter = opentelemetry::exporter::metrics::
        OStreamMetricExporterFactory::Create();

    auto reader = metric_sdk::PeriodicExportingMetricReaderFactory::Create(
        std::move(exporter), ostream_options);

    auto context = metric_sdk::MeterContextFactory::Create();
    context->AddMetricReader(std::move(reader));

    auto u_provider =
        metric_sdk::MeterProviderFactory::Create(std::move(context));
    std::shared_ptr<opentelemetry::metrics::MeterProvider> provider(
        std::move(u_provider));

    metrics_api::Provider::SetMeterProvider(provider);
}

void metrics_handler::create_uint_counter(const std::string& name) {
    std::string counter_name = name + "_counter";
    auto provider = opentelemetry::metrics::Provider::GetMeterProvider();
    auto meter = provider->GetMeter(name);
    m_uint_counters[name] = meter->CreateUInt64Counter(counter_name);
    m_uint_counters[name]->Add(0);
}

void metrics_handler::increment_counter(const message_type msg_type) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_served_request_types.contains(msg_type)) {
        add_uint_counter_value(get_message_string(msg_type), 1);
    }
}

void metrics_handler::add_uint_counter_value(const std::string& name,
                                             uint64_t value) {
    m_uint_counters[name]->Add(value);
}
} // namespace uh::cluster
