#ifndef UH_CLUSTER_METRICS_H
#define UH_CLUSTER_METRICS_H

#include "common/types/magic_enum.hpp"
#include "common/types/magic_enum_switch.hpp"
#include "common/utils/common.h"
#include "third-party/opentelemetry-cpp/api/include/opentelemetry/metrics/meter.h"
#include "third-party/opentelemetry-cpp/api/include/opentelemetry/metrics/provider.h"
#include "third-party/opentelemetry-cpp/exporters/otlp/include/opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_factory.h"
#include "third-party/opentelemetry-cpp/exporters/otlp/include/opentelemetry/exporters/otlp/otlp_grpc_metric_exporter_options.h"
#include "third-party/opentelemetry-cpp/sdk/include/opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h"
#include "third-party/opentelemetry-cpp/sdk/include/opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_options.h"
#include "third-party/opentelemetry-cpp/sdk/include/opentelemetry/sdk/metrics/meter_context_factory.h"
#include "third-party/opentelemetry-cpp/sdk/include/opentelemetry/sdk/metrics/meter_provider_factory.h"
#include <condition_variable>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace uh::cluster {

enum metric_type {
    l1_cache_hit,
    l1_cache_miss,
    l2_cache_hit,
    l2_cache_miss,
    dedupe_set_frag_count,
    dedupe_set_frag_size,
    total_ingested_size_mb,
    total_egressed_size_mb,
    total_effective_size_mb,
    total_size_mb,
    storage_read_fragment_req,
    storage_read_address_req,
    storage_write_req,
    storage_sync_req,
    storage_remove_fragment_req,
    storage_used_req,
    deduplicator_req,
    directory_bucket_list_req,
    directory_bucket_put_req,
    directory_bucket_delete_req,
    directory_bucket_exists_req,
    directory_object_list_req,
    directory_object_put_req,
    directory_object_get_req,
    directory_object_delete_req,
    success,
    failure
};

void measure_message_type(message_type type);
void initialize_metrics_exporter(const std::string& endpoint);

template <metric_type type, typename value_type = uint64_t> class metric {

    using otel_counter_type = std::conditional_t<
        std::is_signed_v<value_type>,
        std::unique_ptr<opentelemetry::metrics::UpDownCounter<value_type>>,
        std::unique_ptr<opentelemetry::metrics::Counter<value_type>>>;

    static otel_counter_type create_counter() {
        const auto name = std::string(magic_enum::enum_name(type));
        auto meter =
            opentelemetry::metrics::Provider::GetMeterProvider()->GetMeter(
                name);
        if constexpr (std::is_same_v<value_type, uint64_t>) {
            return meter->CreateUInt64Counter(name);
        } else if constexpr (std::is_same_v<value_type, int64_t>) {
            return meter->CreateInt64UpDownCounter(name);
        } else if constexpr (std::is_same_v<value_type, double>) {
            return meter->CreateDoubleUpDownCounter(name);
        }
    }

    inline static otel_counter_type& get_counter() {
        static otel_counter_type counter = create_counter();
        return counter;
    }

public:
    metric() = delete;

    static void increase(value_type val) { get_counter()->Add(val); }
};

} // namespace uh::cluster

#endif // UH_CLUSTER_METRICS_H
