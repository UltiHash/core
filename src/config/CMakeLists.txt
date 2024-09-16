add_library(uh_config configuration.cpp)
target_link_libraries(uh_config uh_telemetry uh_utils uh_entrypoint uh_license uh_deduplicator uh_storage uh_db CLI11::CLI11)
