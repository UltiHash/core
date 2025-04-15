add_library(etcd
            registry/service_id.cpp
            registry/service_registry.cpp
            registry/storage_registry.cpp
            namespace.cpp
            utils.cpp)

target_link_libraries(etcd
    etcd-cpp-apiv3
    ec
    service_interfaces
    network
    telemetry)
