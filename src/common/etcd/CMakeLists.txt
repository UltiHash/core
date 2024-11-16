add_library(uh_etcd utils.cpp)
target_link_libraries(uh_etcd PUBLIC uh_utils uh_telemetry uh_storage_interfaces etcd-cpp-api)

add_subdirectory(registry)
add_subdirectory(service_discovery)
add_subdirectory(ec_groups)
