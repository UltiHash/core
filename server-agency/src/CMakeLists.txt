configure_file(config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

add_executable(agency-node main.cpp)
target_link_libraries(agency-node uhoptions)
target_include_directories(agency-node PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
