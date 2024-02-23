#include "metrics.h"

#include "common/utils/common.h"
#include "log.h"

#include <algorithm>

namespace metric_sdk = opentelemetry::sdk::metrics;
namespace common = opentelemetry::common;
namespace metrics_api = opentelemetry::metrics;
namespace otlp_exporter = opentelemetry::exporter::otlp;

namespace uh::cluster {

constexpr metric_sdk::PeriodicExportingMetricReaderOptions otlp_options{
    .export_interval_millis = std::chrono::milliseconds(1000),
    .export_timeout_millis = std::chrono::milliseconds(500)};

void initialize_metrics_exporter(const std::string& endpoint) {

    if (endpoint.empty()) {
        return;
    }

    std::unique_ptr<metric_sdk::MetricReader> reader;
    opentelemetry::exporter::otlp::OtlpGrpcMetricExporterOptions
        exporter_options;
    exporter_options.endpoint = endpoint;
    auto exporter =
        otlp_exporter::OtlpGrpcMetricExporterFactory::Create(exporter_options);
    reader = metric_sdk::PeriodicExportingMetricReaderFactory::Create(
        std::move(exporter), otlp_options);

    auto context = metric_sdk::MeterContextFactory::Create();
    context->AddMetricReader(std::move(reader));

    auto metrics_provider_unique =
        metric_sdk::MeterProviderFactory::Create(std::move(context));
    std::shared_ptr<opentelemetry::metrics::MeterProvider>
        metrics_provider_shared(std::move(metrics_provider_unique));

    metrics_api::Provider::SetMeterProvider(metrics_provider_shared);
}

constexpr metric_type convert_message_type(message_type mtype) {
    auto str = std::string(magic_enum::enum_name(mtype));
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto mt = magic_enum::enum_cast<metric_type>(str);
    if (!mt) [[unlikely]] {
        throw std::runtime_error(
            "Could not convert message type to metric type: " + str);
    }
    return *mt;
}

constexpr void measure_message_type(message_type type) {
    const auto mt = convert_message_type(type);
    magic_enum::enum_switch(
        [](auto mt) {
            constexpr auto cmt = mt;
            metric<cmt>::increase(1);
        },
        mt);
}

} // namespace uh::cluster
