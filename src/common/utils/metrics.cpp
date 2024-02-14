#include "metrics.h"

#include "common/utils/log.h"
#include "opentelemetry/exporters/ostream/metric_exporter_factory.h"

namespace metric_sdk = opentelemetry::sdk::metrics;
namespace common = opentelemetry::common;
namespace metrics_api = opentelemetry::metrics;
namespace otlp_exporter = opentelemetry::exporter::otlp;

namespace uh::cluster {
metrics::metrics() { initialize_metrics_exporter(); }

metrics::metrics(const uh::cluster::role service_role)
    : m_served_request_types(get_requests_served(service_role)) {
    initialize_metrics_exporter();
    for (auto message_type : m_served_request_types) {
        create_uint_counter(get_message_string(message_type));
    }
}

void metrics::initialize_metrics_exporter() {
    // initialize_metrics_otlp_grpc_exporter();
    initialize_metrics_ostream_exporter();
}

void metrics::initialize_metrics_otlp_grpc_exporter() {
    otlp_exporter::OtlpGrpcMetricExporterOptions exporter_options;
    exporter_options.endpoint = "localhost";
    auto exporter =
        otlp_exporter::OtlpGrpcMetricExporterFactory::Create(exporter_options);

    // Initialize and set the global MeterProvider
    metric_sdk::PeriodicExportingMetricReaderOptions reader_options;
    reader_options.export_interval_millis = std::chrono::milliseconds(1000);
    reader_options.export_timeout_millis = std::chrono::milliseconds(500);

    auto reader = metric_sdk::PeriodicExportingMetricReaderFactory::Create(
        std::move(exporter), reader_options);

    auto context = metric_sdk::MeterContextFactory::Create();
    context->AddMetricReader(std::move(reader));

    auto u_provider =
        metric_sdk::MeterProviderFactory::Create(std::move(context));
    std::shared_ptr<opentelemetry::metrics::MeterProvider> provider(
        std::move(u_provider));

    metrics_api::Provider::SetMeterProvider(provider);
}

void metrics::initialize_metrics_ostream_exporter() {
    auto exporter = opentelemetry::exporter::metrics::
        OStreamMetricExporterFactory::Create();

    // Initialize and set the global MeterProvider
    metric_sdk::PeriodicExportingMetricReaderOptions options;
    options.export_interval_millis = std::chrono::milliseconds(1000);
    options.export_timeout_millis = std::chrono::milliseconds(500);

    auto reader = metric_sdk::PeriodicExportingMetricReaderFactory::Create(
        std::move(exporter), options);

    auto context = metric_sdk::MeterContextFactory::Create();
    context->AddMetricReader(std::move(reader));

    auto u_provider =
        metric_sdk::MeterProviderFactory::Create(std::move(context));
    std::shared_ptr<opentelemetry::metrics::MeterProvider> provider(
        std::move(u_provider));

    metrics_api::Provider::SetMeterProvider(provider);
}

void metrics::create_uint_counter(const std::string& name) {
    std::string counter_name = name + "_counter";
    auto provider = opentelemetry::metrics::Provider::GetMeterProvider();
    auto meter = provider->GetMeter(name);
    m_uint_counters[name] = meter->CreateUInt64Counter(counter_name);
    m_uint_counters[name]->Add(0);
}

void metrics::increment_served_request_counter(
    const uh::cluster::message_type msg_type) {
    if (m_served_request_types.contains(msg_type)) {
        add_uint_counter_value(get_message_string(msg_type), 1);
    }
}

void metrics::add_uint_counter_value(const std::string& name, uint64_t value) {
    m_uint_counters[name]->Add(value);
}
} // namespace uh::cluster
