add_library(uh_registry service_registry.cpp service_id.cpp)
target_link_libraries(
    uh_registry
    PRIVATE uh_utils
    PUBLIC uh_network uh_etcd)
