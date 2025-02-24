add_library_bundle(
    etcd
    SOURCES
    utils.cpp
    PUBLIC
    etcd-cpp-api
    PUBLIC_BUNDLE
    telemetry)
