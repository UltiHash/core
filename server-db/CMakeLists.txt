cmake_minimum_required(VERSION 3.22.0)
project(uhServerDB VERSION 0.2.0)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

# generate config header file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp)

# external dependencies
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.80 COMPONENTS regex date_time system filesystem unit_test_framework program_options REQUIRED)

# internal dependencies
set(${PROJECT_NAME}_INTERNAL_DEPS uhLibLogging uhLibUtil uhLibNet uhLibOptions uhLibProtocol uhLibCompression uhLibMetrics uhLibSerialization uhLibSignals)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${Boost_INCLUDE_DIRS} ${${PROJECT_NAME}_INTERNAL_DEPS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${${PROJECT_NAME}_INTERNAL_DEPS})

# adding all the .h files of the current project
file(GLOB_RECURSE HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)

# adding all the .cpp files of the current project
file(GLOB_RECURSE SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_library(${PROJECT_NAME}_OBJECTS OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME}_OBJECTS PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_OBJECTS PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)
