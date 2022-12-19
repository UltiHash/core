cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibCommon)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

option(WITH_DBSUPPORT "Include the dbSupport module in the build of lib-common." OFF)
#SET(WITH_DBSUPPORT ON CACHE BOOL "Override 'WITH_DBSUPPORT' option" FORCE)


#testing needs to be enabled to 'collect' tests of all subdirectories into top-level cmake project
enable_testing()

add_subdirectory(logging)
add_subdirectory(options)
add_subdirectory(conceptTypes)
add_subdirectory(net)
add_subdirectory(protocol)
add_subdirectory(numbers)
add_subdirectory(util)
add_subdirectory(serialization)
add_subdirectory(trees)

if(WITH_DBSUPPORT)
    add_subdirectory(dbSupport)
endif()