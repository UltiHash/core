add_library(uh_registry STATIC service_registry.cpp service_id.cpp)
target_link_libraries(
    uh_registry
    PRIVATE uh_utils
    PUBLIC uh_network uh_etcd)
target_precompile_headers(uh_registry REUSE_FROM uh_precompile_headers)
