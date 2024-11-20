add_library(uh_global_data STATIC global_data_view.cpp)
target_link_libraries(
    uh_global_data
    PRIVATE uh_utils
    PUBLIC uh_caches uh_service_discovery uh_ec_groups uh_types)
target_precompile_headers(uh_global_data REUSE_FROM uh_precompile_headers)
