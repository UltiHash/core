#include "message_metrics_handler.h"

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

message_metrics_handler::message_metrics_handler(
    const std::string& telemetry_endpoint) {
    initialize_metrics_exporter(telemetry_endpoint);

    for (uint8_t msg_num = 0; msg_num != LAST; msg_num++) {
        auto msg_type = static_cast<message_type>(msg_num);
        create_uint_counter(get_message_string(msg_type));
    }
}

void message_metrics_handler::initialize_metrics_exporter(
    const std::string& telemetry_endpoint) {
    std::unique_ptr<metric_sdk::MetricReader> reader;

    if (telemetry_endpoint.empty()) {
        auto exporter = opentelemetry::exporter::metrics::
            OStreamMetricExporterFactory::Create();
        reader = metric_sdk::PeriodicExportingMetricReaderFactory::Create(
            std::move(exporter), ostream_options);
    } else {
        otlp_exporter::OtlpGrpcMetricExporterOptions exporter_options;
        exporter_options.endpoint = telemetry_endpoint;
        auto exporter = otlp_exporter::OtlpGrpcMetricExporterFactory::Create(
            exporter_options);
        reader = metric_sdk::PeriodicExportingMetricReaderFactory::Create(
            std::move(exporter), otlp_options);
    }

    auto context = metric_sdk::MeterContextFactory::Create();
    context->AddMetricReader(std::move(reader));

    auto metrics_provider_unique =
        metric_sdk::MeterProviderFactory::Create(std::move(context));
    std::shared_ptr<opentelemetry::metrics::MeterProvider>
        metrics_provider_shared(std::move(metrics_provider_unique));

    metrics_api::Provider::SetMeterProvider(metrics_provider_shared);
}

void message_metrics_handler::create_uint_counter(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string counter_name = name + "_counter";
    auto provider = opentelemetry::metrics::Provider::GetMeterProvider();
    auto meter = provider->GetMeter(name);
    m_uint_counters[name] = meter->CreateUInt64Counter(counter_name);
    m_uint_counters[name]->Add(0);
}

void message_metrics_handler::increment_counter(const message_type msg_type) {
    if (m_served_request_types.contains(msg_type)) {
        increase_uint_counter(get_message_string(msg_type), 1);
    }
}

void message_metrics_handler::increase_uint_counter(const std::string& name,
                                                    std::uint64_t value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_uint_counters[name]->Add(value);
}
} // namespace uh::cluster
