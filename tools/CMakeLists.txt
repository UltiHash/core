cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibTools)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(Boost_USE_STATIC_LIBS ON)

#testing needs to be enabled to 'collect' tests of all subdirectories into top-level cmake project
enable_testing()

add_subdirectory(dedup-check)
add_subdirectory(subchunking-check)
