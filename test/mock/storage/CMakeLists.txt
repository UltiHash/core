add_library(mock_storage SHARED data_store_mock.cpp)
target_include_directories(mock_storage PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(
    mock_storage
    PRIVATE zpp_bits
            lmdbxx
            magic_enum
            opentelemetry_exporter_otlp_grpc_log
            opentelemetry_exporter_otlp_grpc
            opentelemetry_exporter_otlp_grpc_metrics
            opentelemetry_exporter_ostream_metrics)
