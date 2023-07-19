cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(uhLibClient)
message(STATUS "Building ${PROJECT_NAME}...")


# external dependencies
find_package(Boost 1.80 COMPONENTS unit_test_framework REQUIRED)

set(CMAKE_CXX_STANDARD 20)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INTERNAL_DEPS uhLibLogging uhLibUtil uhLibIo uhLibNet uhLibOptions uhLibProtocol uhLibSerialization uhLibUhv uhLibChunking)
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../  ${Boost_INCLUDE_DIRS} ${OPENSSL_INCLUDE_DIRS} ${${PROJECT_NAME}_INTERNAL_DEPS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} ${${PROJECT_NAME}_INTERNAL_DEPS})


# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)
