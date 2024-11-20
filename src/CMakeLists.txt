add_subdirectory(pch)
add_subdirectory(config)
add_subdirectory(common)
add_subdirectory(entrypoint)
add_subdirectory(deduplicator)
add_subdirectory(storage)
add_subdirectory(recovery)

add_library(uh_libraries INTERFACE)
target_link_libraries(
    uh_libraries
    INTERFACE uh_config
              uh_telemetry
              uh_license
              uh_utils
              uh_entrypoint
              uh_deduplicator
              uh_storage)
