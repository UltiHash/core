add_library(uh_policy STATIC parser.cpp policy.cpp module.cpp variables.cpp)
target_link_libraries(
    uh_policy
    PRIVATE uh_telemetry nlohmann_json
    PUBLIC uh_commands uh_http uh_utils uh_entrypoint)
target_precompile_headers(uh_policy REUSE_FROM uh_precompile_headers)
