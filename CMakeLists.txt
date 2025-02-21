cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

project(uh-cluster VERSION 1.2.1)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_TOOLS "Build tools" ON)
option(ENABLE_DEDUPE_LOG "Build deduplication set log" OFF)
option(ENABLE_TRACE "Write execution trace to log" OFF)
option(ENABLE_BUILD_TIME_TRACE "Build time trace, clang only" OFF)
option(ENABLE_CCACHE "Use cache while compiling" ON)
option(ENABLE_TSAN "Enable thread-sanitizer" OFF)
option(ENABLE_ASAN "Enable address sanitizer" OFF)
option(ENABLE_UBSAN "Enable undefined sanitizer" OFF)

if(ENABLE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        # https://ccache.dev/manual/4.9.1.html#_precompiled_headers
        set(CMAKE_C_COMPILER_LAUNCHER
            "${CCACHE_PROGRAM};namespace=uh-core;sloppiness=pch_defines,time_macros"
        )
        set(CMAKE_CXX_COMPILER_LAUNCHER ${CMAKE_C_COMPILER_LAUNCHER})
    endif()
endif()

if(ENABLE_BUILD_TIME_TRACE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        add_compile_options(-ftime-trace)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        # gcc: use plugin https://github.com/royjacobson/externis
        add_compile_options(-fplugin=externis)
    endif()
endif()

if(ENABLE_TSAN)
    if(ENABLE_ASAN OR ENABLE_UBSAN)
        message(
            FATAL
            "ENABLE_TSAN is mutually exclusive with ENABLE_UBSAN and ENABLE_ASAN"
        )
    endif()

    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

if(ENABLE_TSAN
   OR ENABLE_ASAN
   OR ENABLE_UBSAN)
    add_compile_options(-fno-omit-frame-pointer -fno-optimize-sibling-calls)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_LIST_DIR})

message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-fPIC)
set(Boost_USE_STATIC_LIBS ON)

add_subdirectory(third-party)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third-party/etcd-cpp-apiv3)

find_package(
    Boost 1.87
    COMPONENTS thread log url unit_test_framework
    REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Git REQUIRED)
find_package(PostgreSQL REQUIRED)

find_path(LMDB_INCLUDE_DIR NAMES lmdb.h)
find_library(LMDB_LIBRARY NAMES lmdb)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cmake/" ${CMAKE_PREFIX_PATH})

include(CPM)

CPMAddPackage(
    NAME
    cpr
    GITHUB_REPOSITORY
    libcpr/cpr
    GIT_TAG
    1.9.9
    OPTIONS
    "BUILD_SHARED_LIBS OFF")

if(ENABLE_DEDUPE_LOG)
    message(STATUS "Building log dedupe set")
    add_definitions(-DLOG_DEDUPE_SET)
endif()

if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # auto-ignore build directory
    if(NOT EXISTS ${PROJECT_BINARY_DIR}/.gitignore)
        file(WRITE ${PROJECT_BINARY_DIR}/.gitignore "*")
    endif()

    message(STATUS "Reading git revision")

    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_REVISION)
    string(STRIP "${GIT_REVISION}" PROJECT_VCSID)

    execute_process(
        COMMAND ${GIT_EXECUTABLE} remote get-url origin
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_REPOSITORY)
    string(STRIP "${GIT_REPOSITORY}" PROJECT_REPOSITORY)

    message(STATUS "Using revision ${PROJECT_REPOSITORY} -- ${PROJECT_VCSID}")

endif()

set(${PROJECT_NAME}_INTERNAL_DEPS zpp_bits magic_enum lmdbxx reedsolomon)

set(${PROJECT_NAME}_LINK_LIBRARIES
    Boost::log
    Boost::url
    Boost::filesystem
    ${OPENSSL_LIBRARIES}
    etcd-cpp-api
    CLI11::CLI11
    opentelemetry_exporter_otlp_grpc_log
    opentelemetry_exporter_otlp_grpc
    opentelemetry_exporter_otlp_grpc_metrics
    opentelemetry_exporter_ostream_metrics
    ${${PROJECT_NAME}_INTERNAL_DEPS}
    ${PostgreSQL_LIBRARIES}
    ${LMDB_LIBRARY}
    nlohmann_json
    cpr::cpr)
set(${PROJECT_NAME}_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/third-party
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${${PROJECT_NAME}_INTERNAL_DEPS}
    ${PostgreSQL_INCLUDE_DIRS}
    ${LMDB_INCLUDE_DIR})

add_compile_options(-Wall -Werror)

#
# GCC 13 and before issue warnings when using coroutines. Re-try with a newer
# version and remove if it succeeds.
#
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109224
#
add_compile_options(-Wno-mismatched-new-delete)
add_compile_options(-Wno-odr)
add_link_options(-Wno-odr)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

file(GLOB_RECURSE HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_subdirectory(src/pch)

add_library(uh_objects OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(uh_objects PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(uh_objects PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})
target_precompile_headers(uh_objects REUSE_FROM uh_precompile_headers)

add_library(${PROJECT_NAME}_shared SHARED $<TARGET_OBJECTS:uh_objects>)
target_link_libraries(${PROJECT_NAME}_shared PUBLIC uh_objects)
target_include_directories(
    ${PROJECT_NAME}_shared
    INTERFACE
        $<TARGET_PROPERTY:uh_precompile_headers,INTERFACE_INCLUDE_DIRECTORIES>)

set(CMAKE_BUILD_RPATH "$ORIGIN")

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(${PROJECT_NAME} PRIVATE ${PROJECT_NAME}_shared)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE uh_objects)
endif()

target_precompile_headers(${PROJECT_NAME} REUSE_FROM uh_precompile_headers)

add_subdirectory(scripts)

# Unit tests
if(BUILD_TESTS)
    message(STATUS "Building unit tests")
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_TOOLS)
    message(STATUS "Building tools")
    add_subdirectory(tools)
endif()
