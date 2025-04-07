add_library(etcd
            registry/service_registry.cpp
            registry/service_id.cpp
            utils.cpp)

target_link_libraries(etcd
    etcd-cpp-apiv3
    ec
    service_interfaces
    network
    telemetry)
