cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibTrees)
message(STATUS "Using ${PROJECT_NAME}...")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -fsanitize=address")

SET(USE_TINY_TREE ON CACHE BOOL "Override 'USE_TINY_TREE' option" FORCE)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tree_storage_config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/tree_storage_config.h)

# external dependencies
find_package(Boost 1.80 COMPONENTS log unit_test_framework thread system REQUIRED)
find_package(OpenSSL REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} uhLibLogging uhLibUtil)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} INTERFACE ${HEADER_FILE_LIST} tree_storage.cpp)
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)