cmake_minimum_required(VERSION 3.22.0)
set(CMAKE_CXX_STANDARD 23)

project(uhLibStatistics)
message(STATUS "Using ${PROJECT_NAME}...")

# external dependencies
find_package(Boost 1.80 COMPONENTS log unit_test_framework REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES})

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        )

add_library(${PROJECT_NAME} INTERFACE ${HEADER_FILE_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)