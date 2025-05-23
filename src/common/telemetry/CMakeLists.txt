add_library(telemetry
            trace/trace.cpp
            log.cpp
            metrics.cpp
            otel_log_sink.cpp)

target_compile_options(telemetry PRIVATE -Wno-deprecated-builtins -Wno-deprecated-declarations)

target_link_libraries(telemetry
                      magic_enum
                      Boost::log
                      opentelemetry_exporter_otlp_grpc_log
                      opentelemetry_exporter_otlp_grpc
                      opentelemetry_exporter_otlp_grpc_metrics
                      opentelemetry_exporter_ostream_span
                      opentelemetry_exporter_ostream_metrics)
