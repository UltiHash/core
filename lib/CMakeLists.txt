cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibCommon)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

#testing needs to be enabled to 'collect' tests of all subdirectories into top-level cmake project
enable_testing()

add_subdirectory(third-party)
