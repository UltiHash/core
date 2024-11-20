add_library(uh_telemetry STATIC traces.cpp otel_log_sink.cpp log.cpp metrics.cpp)
target_link_libraries(
    uh_telemetry
    PRIVATE Boost::filesystem
    PUBLIC uh_utils
           magic_enum
           opentelemetry_exporter_otlp_grpc_log
           opentelemetry_exporter_otlp_grpc
           opentelemetry_exporter_otlp_grpc_metrics
           opentelemetry_exporter_ostream_metrics
           Boost::log)
target_precompile_headers(uh_telemetry REUSE_FROM uh_precompile_headers)
