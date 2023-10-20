cmake_minimum_required(VERSION 3.1)
project(Core)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

option(BUILD_SERVER_DB      "Build the server-db component"     OFF)
option(BUILD_SERVER_AGENCY  "Build the server-agency component" OFF)
option(BUILD_CLIENT_SHELL   "Build the client-shell component"  OFF)
option(BUILD_CLIENT_FUSE    "Build the client-fuse component"  OFF)
option(BUILD_TOOLS          "Build the tools component"  OFF)
option(BUILD_CLUSTER        "Build the cluster component"  OFF)
option(ENABLE_E2E_TESTS     "Enable End-to-End tests" OFF)


set(Boost_USE_STATIC_LIBS ON)
add_compile_options(-fPIC)


if((NOT BUILD_SERVER_DB) AND (NOT BUILD_SERVER_AGENCY) AND (NOT BUILD_CLIENT_SHELL) AND (NOT BUILD_CLUSTER) AND (NOT BUILD_CLIENT_FUSE) AND (NOT BUILD_TOOLS))
    SET(BUILD_SERVER_DB ON CACHE BOOL       "Building the server-db component" FORCE)
    SET(BUILD_SERVER_AGENCY ON CACHE BOOL   "Building the server-agency component" FORCE)
    SET(BUILD_CLIENT_SHELL ON CACHE BOOL    "Building the server-client component" FORCE)
    SET(BUILD_CLIENT_FUSE ON CACHE BOOL     "Building the server-fuse component" FORCE)
    SET(BUILD_TOOLS ON CACHE BOOL           "Building the tools component" FORCE)
    SET(BUILD_CLUSTER ON CACHE BOOL         "Building the cluster component" FORCE)
endif()

option(USE_LICENSE_SPRING "Use the LicenseSpring licensing backend" OFF)

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

if(BUILD_TOOLS)
add_subdirectory(tools)
endif()

if(BUILD_SERVER_DB)
add_subdirectory(server-db)
endif()

if(BUILD_SERVER_AGENCY)
add_subdirectory(server-agency)
endif()

if(BUILD_CLIENT_SHELL)
add_subdirectory(client-shell)
endif()

if(BUILD_CLIENT_FUSE)
add_subdirectory(client-fuse)
endif()

if(BUILD_CLUSTER)
    add_subdirectory(cluster)
endif()

add_subdirectory(client-sdk)

enable_testing()

add_subdirectory(testing)

set(CPACK_PACKAGE_NAME "ultihash")
set(CPACK_PACKAGE_VENDOR "UltiHash, Inc.")

set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
SET(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/packaging")

set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/")

set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 2)
set(CPACK_PACKAGE_VERSION_PATCH 0)

set(CPACK_PACKAGE_CONTACT "hello@ultihash.io")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "UltiHash Inc. <hello@ultihash.io>")

set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES)
set(CPACK_STRIP_FILES YES)
set(CPACK_DEBIAN_PACKAGE_SECTION database)
set(CPACK_INSTALL_DEFAULT_DIRECTORY_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)

#set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
#set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
set(CPACK_DEB_COMPONENT_INSTALL YES)

set(CPACK_DEBIAN_UH-CLUSTER_PACKAGE_NAME "ultihash-cluster")
set(CPACK_DEBIAN_UH-CLUSTER_DESCRIPTION "Scale-out storage cluster  for the UltiHash high-performance object storage")

set(CPACK_DEBIAN_UH-CLI_PACKAGE_NAME "ultihash-cli")
set(CPACK_DEBIAN_UH-CLI_DESCRIPTION "CLI for the UltiHash high-performance object storage (single node)")

set(CPACK_DEBIAN_UH-DATA-NODE_PACKAGE_NAME "ultihash-data-node")
set(CPACK_DEBIAN_UH-DATA-NODE_DESCRIPTION "Single-node data node server for the UltiHash high-performance object storage")

include(CPack)
