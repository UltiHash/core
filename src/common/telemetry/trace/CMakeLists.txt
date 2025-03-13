add_library(trace SHARED trace.cpp)
target_include_directories(
  trace PUBLIC ${CMAKE_SOURCE_DIR}/src)

include(CPM)
CPMAddPackage(
  NAME opentelemetry
  GITHUB_REPOSITORY open-telemetry/opentelemetry-cpp
  VERSION 1.19.0
  OPTIONS "BUILD_TESTING OFF" "WITH_OTLP_HTTP OFF" "WITH_OTLP_GRPC ON")

target_link_libraries(trace PRIVATE ${CMAKE_THREAD_LIBS_INIT}
                      common_foo_library)
target_link_libraries(trace PUBLIC opentelemetry_trace
                      opentelemetry_exporter_ostream_span
                      opentelemetry_exporter_otlp_grpc_log
                      opentelemetry_exporter_otlp_grpc
                      opentelemetry_exporter_otlp_grpc_metrics
                      opentelemetry_exporter_ostream_metrics)

