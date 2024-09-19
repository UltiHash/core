project(storage_stress_test)

add_executable(uh-storage-stress-test main.cpp)
target_link_libraries(uh-storage-stress-test uh_common_network)
