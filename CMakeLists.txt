cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

project(uh-cluster VERSION 1.5.0)

#
# Option handling
#

option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_TOOLS "Build tools" ON)
option(BUILD_SHARED_LIBS "Build all libraries as shared libraries" OFF)
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

#
# Compile options for our build and for third-party libraries as well
#
add_compile_options(-fPIC)

#
# External dependencies
#

add_subdirectory(third-party)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

set(Boost_USE_STATIC_LIBS ON)
find_package(
    Boost 1.87
    COMPONENTS thread log url unit_test_framework
    REQUIRED)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/cmake/" ${CMAKE_PREFIX_PATH})

include(CPM)

CPMAddPackage(
    NAME
    cpr
    GITHUB_REPOSITORY
    libcpr/cpr
    GIT_TAG
    1.11.2
    OPTIONS
    "BUILD_SHARED_LIBS OFF;CURL_USE_LIBPSL OFF"
    CPM_ARGS
    "TIMEOUT 300")

CPMAddPackage(
    NAME
    etcd
    GITHUB_REPOSITORY
    UltiHash/etcd-cpp-apiv3
    GIT_TAG
    v0.15.4+ultihash.patch1
    OPTIONS
    "BUILD_SHARED_LIBS OFF;BUILD_ETCD_CORE_ONLY ON"
    CPM_ARGS
    "TIMEOUT 300")

find_package(Git REQUIRED)
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


#
# Compile Options
#
set(CMAKE_CXX_STANDARD 20)
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
add_compile_options(-pthread)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
else()
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION OFF)
endif()

set(CMAKE_BUILD_RPATH_USE_ORIGIN On)


#
# Directory stucture
#
message(STATUS "Building ${PROJECT_NAME} ${PROJECT_VERSION}...")

set(UH_BINARY_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/bin)
file(MAKE_DIRECTORY ${UH_BINARY_OUTPUT_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

add_subdirectory(src)

# Unit tests
if(BUILD_TESTS)
    message(STATUS "Building unit tests")
    enable_testing()
    add_subdirectory(test)
endif()

if(BUILD_TOOLS)
    message(STATUS "Building tools")
    add_subdirectory(tools)
    add_subdirectory(scripts)
endif()
