cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(22_Statistics)
set(CMAKE_CXX_STANDARD 23)

# external dependencies
find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES})

# internal dependencies
#set(DEPENDENCIES logging conceptTypes)
#include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/dependencies/CMakeLists.txt)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST
        ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h
        )

add_library(${PROJECT_NAME} INTERFACE ${HEADER_FILE_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)