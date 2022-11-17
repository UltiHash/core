cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibCommon)

#testing needs to be enabled to 'collect' tests of all subdirectories into top-level cmake project
enable_testing()

set(COMPONENTS network logging conceptTypes numbers statistics dbSupport)
foreach(COMPONENT ${COMPONENTS})
    include_directories(${CMAKE_SOURCE_DIR}/${COMPONENT}/include)
    add_subdirectory(${COMPONENT})
endforeach()