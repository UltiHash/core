add_library(uh_service_interfaces STATIC service_factory.cpp)
target_link_libraries(
    uh_service_interfaces
    PRIVATE uh_dedupe_interfaces uh_storage_interfaces
    PUBLIC uh_utils uh_config uh_types uh_coroutines)
target_precompile_headers(uh_service_interfaces REUSE_FROM uh_precompile_headers)
