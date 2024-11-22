add_subdirectory(registry)
add_subdirectory(service_discovery)
add_subdirectory(ec_groups)

add_static_and_shared(uh_etcd SOURCES utils.cpp PUBLIC uh_utils etcd-cpp-api)
