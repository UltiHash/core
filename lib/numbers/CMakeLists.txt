cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibNumbers)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

option(USE_GMP "Use libgmp instead of a custom implementation for arbitrary precision integer arithmetic." OFF)

# generate config header file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/BigInteger.h.in ${CMAKE_CURRENT_BINARY_DIR}/BigInteger.h)

# external dependencies
find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)
find_path(GMP_INCLUDE_DIRS NAMES gmp.h)
find_library(GMP_LIBRARY NAMES gmp libgmp)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${GMP_INCLUDE_DIRS} ${CMAKE_CURRENT_BINARY_DIR})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${GMP_LIBRARY})

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

add_library(${PROJECT_NAME} INTERFACE ${HEADER_FILE_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)