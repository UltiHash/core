cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibProtocol)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

# external dependencies
find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)

option(PROTOCOL_USE_SMALL_CHUNKS "Enable small maximum chunk size to allow for testing protocol implementation in environments with restricted memory" OFF)

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} uhLibNet uhLibLogging uhLibUtil uhLibSerialization)
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} uhLibNet uhLibLogging uhLibUtil uhLibSerialization)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)

if (PROTOCOL_USE_SMALL_CHUNKS)
    add_compile_definitions(PROTOCOL_MAXIMUM_CHUNK_SIZE=10*1024*1024)
endif()


