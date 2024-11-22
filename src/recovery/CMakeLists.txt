add_library(uh_recovery INTERFACE)
target_link_libraries(
    uh_recovery INTERFACE uh_ec_groups uh_config uh_utils uh_ec
                          uh_service_discovery uh_telemetry)
