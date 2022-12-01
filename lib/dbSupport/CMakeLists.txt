cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibDBSupport)
message(STATUS "Using ${PROJECT_NAME}...")

# external dependencies
find_package(Boost 1.80 COMPONENTS log unit_test_framework REQUIRED)
find_library(PQXX_LIB pqxx REQUIRED)
find_library(PQ_LIB pq REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${PQXX_LIB})

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} STATIC ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC uhLibLogging uhLibConceptTypes ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC uhLibLogging uhLibConceptTypes ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)