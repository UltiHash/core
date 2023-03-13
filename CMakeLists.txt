cmake_minimum_required(VERSION 3.1)
project(Core)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

option(BUILD_SERVER_DB      "Build the server-db component"     OFF)
option(BUILD_SERVER_AGENCY  "Build the server-agency component" OFF)
option(BUILD_CLIENT_SHELL   "Build the client-shell component"  ON)

if((NOT BUILD_SERVER_DB) AND (NOT BUILD_SERVER_AGENCY) AND (NOT BUILD_CLIENT_SHELL))
    SET(BUILD_SERVER_DB ON CACHE BOOL       "Building the server-db component" FORCE)
    SET(BUILD_SERVER_AGENCY ON CACHE BOOL   "Building the server-agency component" FORCE)
    SET(BUILD_CLIENT_SHELL ON CACHE BOOL    "Building the server-client component" FORCE)
endif()

find_package(Git REQUIRED)
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

add_subdirectory(lib)
if(BUILD_SERVER_DB)
add_subdirectory(server-db)
endif()

if(BUILD_SERVER_AGENCY)
add_subdirectory(server-agency)
endif()

if(BUILD_CLIENT_SHELL)
add_subdirectory(client-shell)
endif()

enable_testing()
