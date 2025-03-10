add_library(uh_test_util SHARED server.cpp random.cpp temp_directory.cpp)
target_include_directories(
    uh_test_util
    PRIVATE project_includes
    PUBLIC ${CMAKE_SOURCE_DIR}/test)
target_link_libraries(uh_test_util PUBLIC boost_headers PUBLIC utils_shared)
# target_precompile_headers(uh_test_util REUSE_FROM uh_precompile_headers)
