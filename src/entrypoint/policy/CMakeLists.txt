add_library(uh_policy parser.cpp policy.cpp module.cpp variables.cpp)
target_link_libraries(uh_policy PRIVATE uh_telemetry nlohmann_json PUBLIC uh_commands uh_http uh_utils uh_entrypoint)
