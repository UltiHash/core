add_library(uh_dedupe_interfaces INTERFACE)
target_link_libraries(
    uh_dedupe_interfaces INTERFACE uh_service_interfaces uh_network
                                   uh_global_data uh_coroutines)
