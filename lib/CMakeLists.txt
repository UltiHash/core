cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibCommon)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(Boost_USE_STATIC_LIBS ON)

#testing needs to be enabled to 'collect' tests of all subdirectories into top-level cmake project
enable_testing()

add_subdirectory(logging)
add_subdirectory(options)
add_subdirectory(net)
add_subdirectory(protocol)
add_subdirectory(metrics)
add_subdirectory(numbers)
add_subdirectory(util)
add_subdirectory(trees)
add_subdirectory(third-party)
