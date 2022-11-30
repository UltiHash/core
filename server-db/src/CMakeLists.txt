
file(GLOB SOURCES
        *.h
        *.cpp
        )

find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)

add_executable(main ${SOURCES})

target_link_libraries(main uhLibOptions uhLibLogging ${Boost_LIBRARIES})
target_include_directories(main PRIVATE ${CMAKE_CURRENT_BINARY_DIR})