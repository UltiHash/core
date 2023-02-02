cmake_minimum_required(VERSION 3.1)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

option(BUILD_SERVER_DB      "Build the server-db component"     OFF)
option(BUILD_SERVER_AGENCY  "Build the server-agency component" OFF)
option(BUILD_CLIENT_SHELL   "Build the client-shell component"  OFF)

if((NOT BUILD_SERVER_DB) AND (NOT BUILD_SERVER_AGENCY) AND (NOT BUILD_CLIENT_SHELL))
    SET(BUILD_SERVER_DB ON CACHE BOOL       "Building the server-db component" FORCE)
    SET(BUILD_SERVER_AGENCY ON CACHE BOOL   "Building the server-agency component" FORCE)
    SET(BUILD_CLIENT_SHELL ON CACHE BOOL    "Building the server-client component" FORCE)
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
