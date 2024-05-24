#include "metrics.h"

#include "common/utils/common.h"
#include "log.h"
#include "opentelemetry/sdk/metrics/view/view_registry_factory.h"

#include <algorithm>
#include <config.h>
#include <iostream>

namespace metric_sdk = opentelemetry::sdk::metrics;
namespace common = opentelemetry::common;
namespace metrics_api = opentelemetry::metrics;
namespace otlp_exporter = opentelemetry::exporter::otlp;

namespace uh::cluster {

std::string GDV_PREFIX = "gdv";
std::string COUNTER_SUFFIX = "counter";
std::string REQ_SUFFIX = "req";

std::basic_string<char> get_role_prefix(role svc_role) {
    auto role_str = std::string(magic_enum::enum_name(svc_role));
    std::transform(role_str.begin(), role_str.end(), role_str.begin(),
                   [](unsigned char c) { return tolower(c); });
    role_str = role_str.substr(0, role_str.find("_"));
    return role_str;
}

void initialize_counters() {
    magic_enum::enum_for_each<metric_type>([](auto val) {
        constexpr metric_type type = val;
        if (type == success || type == failure) {
            metric<type>::increase(0);
        } else {
            auto type_str = magic_enum::enum_name(type);
            auto metric_prefix = type_str.substr(0, type_str.find("_"));
            auto metric_suffix = type_str.substr(type_str.rfind("_") + 1);

            std::basic_string<char> role_prefix = get_role_prefix(service_role);
            if ((metric_suffix == COUNTER_SUFFIX or
                 metric_suffix == REQ_SUFFIX) and
                (metric_prefix == role_prefix or
                 (metric_prefix == GDV_PREFIX and
                  (role_prefix == get_role_prefix(DEDUPLICATOR_SERVICE) or
                   role_prefix == get_role_prefix(ENTRYPOINT_SERVICE))))) {
                metric<type>::increase(0);
            }
        }
    });
}

void initialize_metrics_exporter(role role, const std::string& endpoint,
                                 unsigned interval) {

    if (endpoint.empty()) {
        return;
    }

    uh::cluster::service_role = role;

    std::unique_ptr<metric_sdk::MetricReader> reader;
    opentelemetry::exporter::otlp::OtlpGrpcMetricExporterOptions
        exporter_options;
    exporter_options.endpoint = endpoint;
    auto exporter =
        otlp_exporter::OtlpGrpcMetricExporterFactory::Create(exporter_options);

    metric_sdk::PeriodicExportingMetricReaderOptions otlp_options{
        .export_interval_millis = std::chrono::milliseconds(interval),
        .export_timeout_millis = std::chrono::milliseconds(500)};

    reader = metric_sdk::PeriodicExportingMetricReaderFactory::Create(
        std::move(exporter), otlp_options);

    auto views = metric_sdk::ViewRegistryFactory::Create();
    auto resource = opentelemetry::sdk::resource::Resource::Create(
        {{"service.name", PROJECT_NAME},
         {"service.version", PROJECT_VERSION},
         {"service.role", std::string(magic_enum::enum_name(service_role))}});

    auto context =
        metric_sdk::MeterContextFactory::Create(std::move(views), resource);
    context->AddMetricReader(std::move(reader));

    auto metrics_provider_unique =
        metric_sdk::MeterProviderFactory::Create(std::move(context));
    std::shared_ptr<opentelemetry::metrics::MeterProvider>
        metrics_provider_shared(std::move(metrics_provider_unique));

    metrics_api::Provider::SetMeterProvider(metrics_provider_shared);

    initialize_counters();
}

constexpr metric_type convert_message_type(message_type mtype) {
    auto str = std::string(magic_enum::enum_name(mtype));
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    auto mt = magic_enum::enum_cast<metric_type>(str);
    if (!mt) [[unlikely]] {
        throw std::runtime_error(
            "Could not convert message type to metric type: " +
            std::to_string(mtype) + ":" + str);
    }
    return *mt;
}

void measure_message_type(message_type type) {
    const auto mt = convert_message_type(type);
    magic_enum::enum_switch(
        [](auto mt) {
            constexpr auto cmt = mt;
            metric<cmt>::increase(1);
        },
        mt);
}

} // namespace uh::cluster
