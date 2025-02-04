add_library(telemetry SHARED context.cpp log.cpp metrics.cpp otel_log_sink.cpp)
target_link_libraries(
    telemetry
    PUBLIC project_includes
           boost_headers
           Boost::log
           opentelemetry_exporter_otlp_grpc_log
           opentelemetry_exporter_otlp_grpc
           opentelemetry_exporter_otlp_grpc_metrics
           opentelemetry_exporter_ostream_metrics
           common_utils
           magic_enum)
# target_precompile_headers(telemetry REUSE_FROM uh_precompile_headers)
