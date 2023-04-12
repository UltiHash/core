cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibOptions)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

# external dependencies
find_package(Boost 1.80 COMPONENTS program_options unit_test_framework REQUIRED)

# internal dependencies
set(${PROJECT_NAME}_INTERNAL_DEPS uhLibLogging uhLibUtil uhLibNet uhLibProtocol uhLibMetrics)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INTERNAL_DEPS} ${Boost_INCLUDE_DIRS} uhLibSerialization)
set(${PROJECT_NAME}_LINK_LIBRARIES ${${PROJECT_NAME}_INTERNAL_DEPS} ${Boost_LIBRARIES} uhLibSerialization)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)
