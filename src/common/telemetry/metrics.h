#pragma once

#include "common/utils/common.h"

#include <magic_enum/magic_enum.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#ifdef __clang__
#pragma GCC diagnostic ignored "-Wdeprecated-builtins"
#endif

#include <opentelemetry/metrics/meter.h>
#include <opentelemetry/metrics/provider.h>
#include <opentelemetry/nostd/shared_ptr.h>
#include <opentelemetry/sdk/metrics/export/periodic_exporting_metric_reader_factory.h>

#include <opentelemetry/sdk/metrics/meter_provider_factory.h>

#pragma GCC diagnostic pop

#include <condition_variable>
#include <string>

namespace uh::cluster {

enum metric_type {
    gdv_l1_cache_hit_counter,
    gdv_l1_cache_miss_counter,
    gdv_l2_cache_hit_counter,
    gdv_l2_cache_miss_counter,
    deduplicator_set_fragment_counter,
    deduplicator_set_fragment_size_counter,
    entrypoint_ingested_data_counter,
    entrypoint_egressed_data_counter,
    entrypoint_original_data_volume_gauge,
    storage_available_space_gauge,
    storage_used_space_gauge,
    storage_read_address_req,
    storage_read_req,
    storage_write_req,
    storage_link_req,
    storage_unlink_req,
    storage_used_req,
    storage_allocate_req,
    storage_get_refcounts_req,
    deduplicator_req,
    entrypoint_abort_multipart_req,
    entrypoint_complete_multipart_req,
    entrypoint_create_bucket_req,
    entrypoint_delete_bucket_req,
    entrypoint_delete_object_req,
    entrypoint_delete_objects_req,
    entrypoint_get_metrics_req,
    entrypoint_get_ready_req,
    entrypoint_get_license_info_req,
    entrypoint_get_object_req,
    entrypoint_head_object_req,
    entrypoint_init_multipart_req,
    entrypoint_list_buckets_req,
    entrypoint_list_multipart_req,
    entrypoint_list_objects_req,
    entrypoint_list_objects_v2_req,
    entrypoint_multipart_req,
    entrypoint_put_object_req,
    active_connections,
    success,
    failure
};

enum metric_unit {
    count,
    byte,
    mebibyte,
};

// unfortunately we need this manual conversion, as "1" is not a valid enum
// value, as is sth. like MiB/s
constexpr std::string get_unit_string(metric_unit unit) {
    switch (unit) {
    case count:
        return "1";
    case byte:
        return "byte";
    case mebibyte:
        return "MiB";
    }
    return "";
}

void measure_message_type(message_type type);
void initialize_metrics_exporter(const std::string& endpoint,
                                 unsigned interval);

template <metric_type type, metric_unit unit = count,
          typename value_type = uint64_t>
class metric {

    inline static std::function<value_type()> m_gauge_cb;
    inline static std::function<
        std::vector<std::pair<std::string, std::string>>()>
        m_label_cb;

    using otel_counter_type = std::conditional_t<
        std::is_signed_v<value_type>,
        std::unique_ptr<opentelemetry::metrics::UpDownCounter<value_type>>,
        std::unique_ptr<opentelemetry::metrics::Counter<value_type>>>;

    using otel_gauge_type = opentelemetry::nostd::shared_ptr<
        opentelemetry::metrics::ObservableInstrument>;

    static otel_counter_type create_counter() {
        const auto name = std::string(magic_enum::enum_name(type));
        const auto service_name =
            std::string(magic_enum::enum_name(global_service_role));
        auto meter =
            opentelemetry::metrics::Provider::GetMeterProvider()->GetMeter(
                service_name);
        if constexpr (std::is_same_v<value_type, uint64_t>) {
            return meter->CreateUInt64Counter(name, "", get_unit_string(unit));
        } else if constexpr (std::is_same_v<value_type, int64_t>) {
            return meter->CreateInt64UpDownCounter(name, "",
                                                   get_unit_string(unit));
        } else if constexpr (std::is_same_v<value_type, double>) {
            return meter->CreateDoubleUpDownCounter(name, "",
                                                    get_unit_string(unit));
        }
    }

    static otel_gauge_type create_gauge() {
        const auto name = std::string(magic_enum::enum_name(type));
        const auto service_name =
            std::string(magic_enum::enum_name(global_service_role));
        auto meter =
            opentelemetry::metrics::Provider::GetMeterProvider()->GetMeter(
                service_name);
        if constexpr (std::is_same_v<value_type, int64_t>) {
            return meter->CreateInt64ObservableGauge(name, "",
                                                     get_unit_string(unit));
        } else if constexpr (std::is_same_v<value_type, double>) {
            return meter->CreateDoubleObservableGauge(name, "",
                                                      get_unit_string(unit));
        }
    }

    inline static otel_counter_type& get_counter() {
        static otel_counter_type counter = create_counter();
        return counter;
    }

    inline static otel_gauge_type& get_gauge() {
        static otel_gauge_type gauge = create_gauge();
        return gauge;
    }

    static void
    gauge_callback_wrapper(opentelemetry::metrics::ObserverResult observer,
                           void*) {
        auto observer_typed =
            opentelemetry::nostd::get<opentelemetry::nostd::shared_ptr<
                opentelemetry::metrics::ObserverResultT<value_type>>>(observer);
        observer_typed->Observe(m_gauge_cb(), m_label_cb());
    }

public:
    metric() = delete;

    static void increase(value_type val) { get_counter()->Add(val); }
    static void decrease(value_type val) { get_counter()->Add(-val); }
    static void register_gauge_callback(
        std::function<value_type()> fun,
        std::function<std::vector<std::pair<std::string, std::string>>()>
            label_fun) {
        m_gauge_cb = fun;
        m_label_cb = label_fun;
        get_gauge()->AddCallback(gauge_callback_wrapper, nullptr);
    }

    static void remove_gauge_callback() {
        get_gauge()->RemoveCallback(gauge_callback_wrapper, nullptr);
    }
};

template <metric_type type> struct counter_guard {
    counter_guard() { metric<type, count, int64_t>::increase(1); }

    ~counter_guard() { metric<type, count, int64_t>::decrease(1); }
};

} // namespace uh::cluster
