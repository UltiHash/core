add_library_bundle(
    etcd
    SOURCES
    registry/service_registry.cpp
    registry/service_id.cpp
    utils.cpp
    PUBLIC
    etcd-cpp-api
    ec
    service_interfaces
    PUBLIC_BUNDLE
    network
    telemetry)


