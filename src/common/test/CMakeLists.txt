add_library(uh_test server.cpp)
target_link_libraries(uh_test PUBLIC Boost::unit_test_framework)
target_precompile_headers(uh_test REUSE_FROM uh_precompile_headers)
