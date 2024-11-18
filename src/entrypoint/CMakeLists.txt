add_subdirectory(commands)
add_subdirectory(http)
add_subdirectory(user)
add_subdirectory(policy)
add_subdirectory(aws)

add_library(
    uh_entrypoint
    command_factory.cpp
    formats.cpp
    handler.cpp
    utils.cpp
    directory.cpp
    multipart_state.cpp
    service.cpp
    limits.cpp)
target_link_libraries(
    uh_entrypoint
    PRIVATE uh_telemetry uh_commands uh_iam uh_debug
    PUBLIC Boost::url
           # Boost::asio
           uh_deduplicator
           uh_registry
           uh_policy
           uh_http
           uh_types
           uh_utils
           uh_network
           uh_db
           uh_commands
           uh_service_discovery
           uh_service_interfaces
           uh_global_data
           uh_user)
