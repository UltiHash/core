add_library(uh_config STATIC configuration.cpp)
target_link_libraries(
    uh_config
    PUBLIC uh_telemetry
           uh_storage
           uh_license
           uh_utils
           uh_db
           uh_entrypoint
           uh_deduplicator
           CLI11::CLI11)
