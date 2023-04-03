cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibMetrics)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

# internal dependencies
set(${PROJECT_NAME}_INTERNAL_DEPS uhLibUtil uhLibNet uhLibProtocol)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INTERNAL_DEPS} uhLibSerialization2)
set(${PROJECT_NAME}_LINK_LIBRARIES ${${PROJECT_NAME}_INTERNAL_DEPS} uhLibSerialization2)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES} prometheus-cpp::core prometheus-cpp::pull)

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)
