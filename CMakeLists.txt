cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

add_subdirectory(lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/lib/third-party/etcd-cpp-apiv3)

project(uh-cluster VERSION 1.0.0)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_TOOLS "Build tools" ON)
option(BUILD_EXPERIMENTS "Build experiments" OFF)
option(ENABLE_DEDUPE_LOG "Build deduplication set log" OFF)
option(ENABLE_TRACE "Write execution trace to log" OFF)
option(DISABLE_DELETE "Disables reclamation of deleted disk space, only object meta-data will be deleted" OFF)

message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-fPIC)

set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.80 COMPONENTS thread log url unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Git REQUIRED)
find_package(PostgreSQL REQUIRED)

find_path(LMDB_INCLUDE_DIR NAMES lmdb.h)
find_library(LMDB_LIBRARY NAMES lmdb)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cmake/" ${CMAKE_PREFIX_PATH})

if(ENABLE_DEDUPE_LOG)
    message(STATUS "Building log dedupe set")
    add_definitions(-DLOG_DEDUPE_SET)
endif()

if(DISABLE_DELETE)
    message(STATUS "Disabled deletion in storage backend")
    add_definitions(-DDISABLE_STORAGE_REFCOUNT)
endif()

if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # auto-ignore build directory
    if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
        file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
    endif()

    message(STATUS "Reading git revision")

    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_REVISION)
    string(STRIP "${GIT_REVISION}" PROJECT_VCSID)

    execute_process(COMMAND ${GIT_EXECUTABLE} remote get-url origin
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_REPOSITORY)
    string(STRIP "${GIT_REPOSITORY}" PROJECT_REPOSITORY)

    message(STATUS "Using revision ${PROJECT_REPOSITORY} -- ${PROJECT_VCSID}")

endif()

set(${PROJECT_NAME}_INTERNAL_DEPS zpp_bits magic_enum lmdbxx reedsolomon)

set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} etcd-cpp-api CLI11::CLI11 opentelemetry_exporter_otlp_grpc_log opentelemetry_exporter_otlp_grpc opentelemetry_exporter_otlp_grpc_metrics opentelemetry_exporter_ostream_metrics ${${PROJECT_NAME}_INTERNAL_DEPS} ${PostgreSQL_LIBRARIES} ${LMDB_LIBRARY} nlohmann_json)
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS} ${${PROJECT_NAME}_INTERNAL_DEPS} ${PostgreSQL_INCLUDE_DIRS} ${LMDB_INCLUDE_DIR})

add_compile_options(-Wall -Werror)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/test/test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.h)

file(GLOB_RECURSE HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_library(${PROJECT_NAME}_OBJECTS OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME}_OBJECTS PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_OBJECTS PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})

add_subdirectory(scripts)

# Unit tests
if(BUILD_TESTS)
    message(STATUS "Building unit tests")
    enable_testing()
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/testing/CMakeLists.txt)
endif()
if(BUILD_EXPERIMENTS)
    message(STATUS "Building experiments")
    add_subdirectory(experiments)
endif()
if(BUILD_TOOLS)
    message(STATUS "Building tools")
    add_subdirectory(tools)
endif()

