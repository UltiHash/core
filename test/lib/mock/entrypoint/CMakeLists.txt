add_library(uh_mock_entrypoint SHARED entrypoint.cpp)
target_include_directories(
    uh_mock_entrypoint
    PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS}
    PUBLIC ${CMAKE_SOURCE_DIR}/test)
target_link_libraries(
    uh_mock_entrypoint
    PUBLIC ${PROJECT_NAME}_shared)
target_precompile_headers(uh_mock_entrypoint REUSE_FROM uh_precompile_headers)
