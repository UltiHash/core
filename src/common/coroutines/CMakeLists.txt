add_library(uh_coroutines INTERFACE)
target_link_libraries(
    uh_coroutines
    INTERFACE opentelemetry_api
              # opentelemetry_exporter_otlp_grpc_log
              # opentelemetry_exporter_otlp_grpc
              # opentelemetry_exporter_otlp_grpc_metrics
              # opentelemetry_exporter_ostream_metrics
              uh_debug)
# target_precompile_headers(uh_coroutines INTERFACE <boost/asio.hpp>)

# include(CMakePrintHelpers) cmake_print_properties(TARGETS uh_coroutine
# PROPERTIES INTERFACE_INCLUDE_DIRECTORIES INCLUDE_DIRECTORIES)
