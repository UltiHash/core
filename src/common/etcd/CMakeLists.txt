add_library(etcd
            registry/service_id.cpp
            registry/service_registry.cpp
            utils.cpp)

target_link_libraries(etcd
    etcd-cpp-api
    ec
    service_interfaces
    network
    telemetry)
