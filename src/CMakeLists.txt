add_subdirectory(pch)
add_subdirectory(common)
add_subdirectory(config)
add_subdirectory(storage)
add_subdirectory(deduplicator)
add_subdirectory(entrypoint)
add_subdirectory(coordinator)

add_executable(uh-cluster main.cpp)
target_link_libraries(uh-cluster Threads::Threads types telemetry conf entrypoint deduplicator storage coordinator)
set_target_properties(uh-cluster PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

target_precompile_headers(uh-cluster REUSE_FROM uh_precompile_headers)
