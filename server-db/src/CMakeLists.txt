configure_file(config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

file(GLOB SOURCES
        *.h
        *.cpp
        )

find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)

add_executable(db-node ${SOURCES})

target_link_libraries(db-node uhLibOptions uhLibLogging uhLibNetwork uhprotocol ${Boost_LIBRARIES})
target_include_directories(db-node PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
