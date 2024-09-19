project(dd-stress-test)

add_executable(uh-dd-stress-test main.cpp)
target_link_libraries(uh-dd-stress-test uh_network)
