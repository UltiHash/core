add_library(fake_storage SHARED fake_data_store.cpp)
target_include_directories(fake_storage PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(
    fake_storage
    PRIVATE zpp_bits
            lmdbxx
            magic_enum
            opentelemetry_exporter_otlp_grpc_log
            opentelemetry_exporter_otlp_grpc
            opentelemetry_exporter_otlp_grpc_metrics
            opentelemetry_exporter_ostream_metrics)

add_library(fake_global_data SHARED fake_global_data_view.cpp)
target_include_directories(fake_global_data
                           PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(
    fake_global_data
    PRIVATE zpp_bits
            lmdbxx
            magic_enum
            opentelemetry_exporter_otlp_grpc_log
            opentelemetry_exporter_otlp_grpc
            opentelemetry_exporter_otlp_grpc_metrics
            opentelemetry_exporter_ostream_metrics
    PUBLIC fake_storage)
