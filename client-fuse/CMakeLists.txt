cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhClientFuse VERSION 0.1.0)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
message(${CMAKE_MODULE_PATH})


find_package(FUSE REQUIRED)

# generate config header file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

# internal dependencies
set(${PROJECT_NAME}_INTERNAL_DEPS uhLibLogging uhLibUtil uhLibNet uhLibOptions uhLibProtocol)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${${PROJECT_NAME}_INTERNAL_DEPS} ${FUSE_INCLUDE_DIR})
set(${PROJECT_NAME}_LINK_LIBRARIES ${${PROJECT_NAME}_INTERNAL_DEPS} ${FUSE_LIBRARIES})

# adding all the .h files of the current project
file(GLOB_RECURSE HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
# adding all the .cpp files of the current project

#file(GLOB_RECURSE SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
#list(REMOVE_ITEM SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

#add_library(${PROJECT_NAME}_OBJECTS OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
#target_include_directories(${PROJECT_NAME}_OBJECTS PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
#include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)
