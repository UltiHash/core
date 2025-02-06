add_library_bundle(
    etcd
    SOURCES
    utils.cpp
    INTERFACE
    etcd-cpp-api
    PUBLIC_BUNDLE
    telemetry)
