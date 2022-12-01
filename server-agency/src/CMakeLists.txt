configure_file(config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)


add_executable(agency-node
               main.cpp
               options.cpp
               protocol.cpp
               protocol_factory.cpp)

target_link_libraries(agency-node uhLibOptions uhLibLogging ${Boost_LIBRARIES} uhprotocol uhLibNetwork)
target_include_directories(agency-node PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
