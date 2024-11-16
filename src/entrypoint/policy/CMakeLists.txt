add_library(uh_policy parser.cpp policy.cpp module.cpp variables.cpp)
target_link_libraries(uh_policy uh_telemetry uh_commands uh_http uh_utils uh_entrypoint nlohmann_json reedsolomon)
