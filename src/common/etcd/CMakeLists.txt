add_subdirectory(registry)
add_subdirectory(service_discovery)
add_subdirectory(ec_groups)

add_library(uh_etcd STATIC utils.cpp)
target_link_libraries(uh_etcd PUBLIC uh_utils etcd-cpp-api)
