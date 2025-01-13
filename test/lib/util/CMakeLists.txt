add_library(uh_test_util SHARED server.cpp)
target_include_directories(
    uh_test_util
    PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS}
    PUBLIC ${CMAKE_SOURCE_DIR}/test)
target_link_libraries(
    uh_test_util
    PUBLIC ${PROJECT_NAME}_shared)
target_precompile_headers(uh_test_util REUSE_FROM uh_precompile_headers)
