add_library(uh_test_util server.cpp random.cpp temp_directory.cpp)
target_link_libraries(uh_test_util types utils)
# target_precompile_headers(uh_test_util REUSE_FROM uh_precompile_headers)
