add_library_bundle(
    etcd
    SOURCES
    utils.cpp
    INTERFACE
    etcd-cpp-api
    PUBLIC_BUNDLE
    telemetry)

# TODO: I did not include ec_groups, registry, and service_discovery because they create circular dependencies.
#       In my opinion, they should be moved to a separate library.
#       How about moving `etcd/service_discovery` and `service_interfaces/` to `common/service_manager`?
