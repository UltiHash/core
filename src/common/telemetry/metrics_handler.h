#ifndef UH_CLUSTER_METRICS_HANDLER_H
#define UH_CLUSTER_METRICS_HANDLER_H

#include "common/utils/common.h"

#include "third-party/opentelemetry-cpp/api/include/opentelemetry/metrics/meter.h"
#include "third-party/opentelemetry-cpp/api/include/opentelemetry/metrics/provider.h"
#include "third-party/opentelemetry-cpp/exporters/otlp/include/opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_factory.h"
#include "third-party/opentelemetry-cpp/exporters/otlp/include/opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_options.h"
#include "third-party/opentelemetry-cpp/sdk/include/opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h"
#include "third-party/opentelemetry-cpp/sdk/include/opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h"
#include "third-party/opentelemetry-cpp/sdk/include/opentelemetry/sdk/metrics/meter_context_factory.h"
#include "third-party/opentelemetry-cpp/sdk/include/opentelemetry/sdk/metrics/meter_provider_factory.h"

#include <string>
#include <unordered_map>
#include <unordered_set>

namespace uh::cluster {

class metrics_handler {
public:
    metrics_handler(const std::string& telemetry_endpoint);

    void create_uint_counter(const std::string& name);
    void increase_uint_counter(const std::string& name, std::uint64_t value);
    void increment_counter(uh::cluster::message_type msg_type);

private:
    std::unordered_set<uh::cluster::message_type> m_served_request_types;
    std::unordered_map<
        std::string, std::unique_ptr<opentelemetry::metrics::Counter<uint64_t>>>
        m_uint_counters;
    std::mutex m_mutex;

    static void
    initialize_metrics_exporter(const std::string& telemetry_endpoint);
};

} // namespace uh::cluster

#endif // UH_CLUSTER_METRICS_HANDLER_H
