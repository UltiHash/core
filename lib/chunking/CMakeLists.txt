cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibChunking)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

find_package(Boost 1.80 COMPONENTS system unit_test_framework REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS "")
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} uhLibIo)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_C_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
message(STATUS "Chunking source files: ${SOURCE_FILE_LIST}")

add_library(${PROJECT_NAME} OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST} ${SOURCE_FILE_C_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)
