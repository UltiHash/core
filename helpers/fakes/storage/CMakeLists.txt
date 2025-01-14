add_library(uh_fake_storage SHARED fake_data_store.cpp)
target_include_directories(
    uh_fake_storage
    PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS}
    PUBLIC ${CMAKE_SOURCE_DIR}/helpers)
target_link_libraries(
    uh_fake_storage
    PRIVATE zpp_bits
            lmdbxx
            magic_enum
            Boost::log
            Boost::url
            Boost::filesystem
            opentelemetry_exporter_otlp_grpc_log
            opentelemetry_exporter_otlp_grpc
            opentelemetry_exporter_otlp_grpc_metrics
            opentelemetry_exporter_ostream_metrics)

add_library(uh_fake_global_data SHARED fake_global_data_view.cpp)
target_include_directories(
    uh_fake_global_data
    PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS}
    PUBLIC ${CMAKE_SOURCE_DIR}/helpers)
target_link_libraries(
    uh_fake_global_data
    PRIVATE zpp_bits
            lmdbxx
            magic_enum
            reedsolomon
            CLI11::CLI11
            Boost::log
            Boost::url
            Boost::filesystem
            opentelemetry_exporter_otlp_grpc_log
            opentelemetry_exporter_otlp_grpc
            opentelemetry_exporter_otlp_grpc_metrics
            opentelemetry_exporter_ostream_metrics
    PUBLIC uh_fake_storage)
