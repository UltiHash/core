cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
project(UDB VERSION 0.1.0)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-fPIE)

# external dependencies
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.80 COMPONENTS program_options unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)

# internal dependencies
set(${PROJECT_NAME}_INTERNAL_DEPS uhLibNet uhLibProtocol uhLibIo)

# configuring library dependencies for cmake and linker
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} src ${${PROJECT_NAME}_INTERNAL_DEPS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${${PROJECT_NAME}_INTERNAL_DEPS})

# adding all the .h files of the current project
file(GLOB_RECURSE HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/include/*.h)
# adding all the .cpp files of the current project
file(GLOB_RECURSE SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

# static library
add_library(${PROJECT_NAME} STATIC ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})

add_executable(simple_example ${CMAKE_CURRENT_SOURCE_DIR}/examples/simple_example.cpp)
target_link_libraries(simple_example ${PROJECT_NAME})
target_include_directories(simple_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

## create testing targets
#include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/testing/CMakeLists.txt)
