cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

add_subdirectory(lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib
                    ${CMAKE_CURRENT_SOURCE_DIR}/lib/third-party/etcd-cpp-apiv3)

project(uh-cluster VERSION 1.1.0)

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_TOOLS "Build tools" ON)
option(BUILD_EXPERIMENTS "Build experiments" OFF)
option(ENABLE_DEDUPE_LOG "Build deduplication set log" OFF)
option(ENABLE_TRACE "Write execution trace to log" OFF)
option(ENABLE_BUILD_TIME_TRACE "Build time trace, clang only" OFF)
option(ENABLE_CCACHE "Use cache while compiling" ON)

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

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(
    copy-compile-commands ALL
    ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/compile_commands.json ${CMAKE_CURRENT_LIST_DIR})

message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)

add_compile_options(-fPIC) # made compile options same to use pch
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie")

set(Boost_USE_STATIC_LIBS ON)
find_package(
    Boost 1.80
    COMPONENTS log url unit_test_framework
    REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Git REQUIRED)
find_package(PostgreSQL REQUIRED)

find_path(LMDB_INCLUDE_DIR NAMES lmdb.h)
find_library(LMDB_LIBRARY NAMES lmdb)

if(NOT LMDB_INCLUDE_DIR OR NOT LMDB_LIBRARY)
    message(
        FATAL_ERROR "LMDB not found. Ensure lmdb is installed and accessible.")
endif()

# Create an Interface Library for LMDB
add_library(lmdb INTERFACE)

# Set include directories and libraries for the interface library
target_include_directories(lmdb INTERFACE ${LMDB_INCLUDE_DIR})
target_link_libraries(lmdb INTERFACE ${LMDB_LIBRARY})

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cmake/" ${CMAKE_PREFIX_PATH})

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

add_compile_options(-Wall -Winvalid-pch -Werror)

#
# GCC 13 and before issue warnings when using coroutines. Re-try with a newer
# version and remove if it succeeds.
#
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109224
#
add_compile_options(-Wno-mismatched-new-delete)

set(CMAKE_OPTIMIZE_DEPENDENCIES ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_subdirectory(scripts)

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_BINARY_DIR})
include_directories(${Boost_INCLUDE_DIRS})

# Force compile options same to use project-wise pch
add_compile_options(-pthread) # made compile options same to use pch
add_compile_definitions(BOOST_LOG_NO_LIB)
add_compile_definitions(BOOST_URL_NO_LIB)
add_compile_definitions(BOOST_FILESYSTEM_NO_LIB)
add_compile_definitions(BOOST_THREAD_NO_LIB)
get_target_property(definitions opentelemetry_api INTERFACE_COMPILE_DEFINITIONS)
add_compile_definitions(${definitions})

include(modules/AddStaticAndShared)

add_subdirectory(src bin)

add_executable(uh-cluster src/main.cpp)
target_link_libraries(uh-cluster uh_libraries)
target_precompile_headers(uh-cluster REUSE_FROM uh_precompile_headers)

# Unit tests
# if(BUILD_TESTS)
#     message(STATUS "Building unit tests")
#     enable_testing()
#     add_subdirectory(test)
# endif()
if(BUILD_EXPERIMENTS)
    message(STATUS "Building experiments")
    add_subdirectory(experiments)
endif()
# if(BUILD_TOOLS)
#     message(STATUS "Building tools")
#     add_subdirectory(tools)
# endif()
