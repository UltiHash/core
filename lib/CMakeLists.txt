cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibCommon)

#testing needs to be enabled to 'collect' tests of all subdirectories into top-level cmake project
enable_testing()

add_subdirectory(network)
add_subdirectory(logging)
add_subdirectory(conceptTypes)
#add_subdirectory(dbSupport)
add_subdirectory(numbers)
#add_subdirectory(statistics)
add_subdirectory(options)
