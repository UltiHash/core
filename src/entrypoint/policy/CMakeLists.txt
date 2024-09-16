add_library(uh_policy policy.cpp parser.cpp module.cpp)
target_link_libraries(uh_policy uh_telemetry uh_entrypoint uh_commands uh_http nlohmann_json reedsolomon)
