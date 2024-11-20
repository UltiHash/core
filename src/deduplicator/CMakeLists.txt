add_library(uh_deduplicator STATIC deduplicator_handler.cpp fragmentation.cpp)
target_link_libraries(
    uh_deduplicator
    PRIVATE uh_debug uh_coroutines
    PUBLIC uh_telemetry
           uh_registry
           uh_dedupe_interfaces
           uh_service_interfaces
           uh_storage
           uh_types
           uh_utils
           uh_global_data
           uh_dedupe_set
           uh_network)
target_precompile_headers(uh_deduplicator REUSE_FROM uh_precompile_headers)

add_subdirectory(dedupe_set)
add_subdirectory(interfaces)
