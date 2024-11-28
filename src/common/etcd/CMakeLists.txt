add_subdirectory(registry)
add_subdirectory(service_discovery)
add_subdirectory(ec_groups)

add_static_and_shared(
    uh_etcd
    SOURCES
    utils.cpp
    address_utils.cpp
    PUBLIC
    etcd-cpp-api
    uh_ec
    uh_registry
    uh_service_discovery
    uh_ec_groups)
