add_library(uh_storage_interfaces INTERFACE)
target_link_libraries(
    uh_storage_interfaces
    INTERFACE uh_recovery
              uh_ec_groups
              uh_service_discovery
              uh_utils
              uh_storage
              uh_network
              uh_service_interfaces
              uh_etcd
              uh_ec
              uh_coroutines)
