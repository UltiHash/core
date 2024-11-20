add_library(uh_test server.cpp)
target_link_libraries(uh_test PUBLIC Boost::unit_test_framework)
