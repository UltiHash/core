add_library(uh_storage_interfaces INTERFACE)
target_link_libraries(uh_storage_interfaces INTERFACE uh_ec uh_ec_groups # circular reference between modules
uh_service_discovery
)
