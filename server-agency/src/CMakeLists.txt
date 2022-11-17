configure_file(config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)


add_executable(agency-node
               main.cpp
               protocol.cpp
               scheduler.cpp
               server.cpp)

target_link_libraries(agency-node uhLibOptions uhLibLogging ${Boost_LIBRARIES})
target_include_directories(agency-node PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
