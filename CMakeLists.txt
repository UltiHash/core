cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)

add_subdirectory(lib)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib ${CMAKE_CURRENT_SOURCE_DIR}/lib/third-party/etcd-cpp-apiv3)

project(uh-cluster VERSION 0.1.0)
message(STATUS "Building ${PROJECT_NAME}...")

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-fPIC)
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost 1.80 COMPONENTS thread log url unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(prometheus-cpp CONFIG REQUIRED)
find_package(PugiXML REQUIRED)

option(ENABLE_INTEGRATION_TESTS "Enable integration tests" OFF)

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

set(${PROJECT_NAME}_INTERNAL_DEPS zpp_bits)
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} prometheus-cpp::pull pugixml::shared ${PugiXML_LIBRARIES} etcd-cpp-api ${${PROJECT_NAME}_INTERNAL_DEPS})
set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/include ${Boost_INCLUDE_DIRS} ${${PROJECT_NAME}_INTERNAL_DEPS})
add_compile_options(-Wall -Werror)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

file(GLOB_RECURSE HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.h)
file(GLOB_RECURSE SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
list(REMOVE_ITEM SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

add_library(${PROJECT_NAME}_OBJECTS OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME}_OBJECTS PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME}_OBJECTS PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})

add_executable(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_OBJECTS ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} PRIVATE ${${PROJECT_NAME}_INCLUDE_DIRS})

# Testing
enable_testing()
if (${ENABLE_INTEGRATION_TESTS})
    message(STATUS "Configuring the integration test suite...")
    add_subdirectory(testing)
else()
    message(STATUS "Configuring the unit test suite...")
    include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/testing/CMakeLists.txt)
endif()

add_subdirectory(experiments)

# Packaging
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/packaging/CMakeLists.txt)

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

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_COMPONENTS_GROUPING ONE_PER_GROUP)
set(CPACK_DEB_COMPONENT_INSTALL YES)

set(CPACK_DEBIAN_UH-CLUSTER_PACKAGE_NAME "ultihash-cluster")
set(CPACK_DEBIAN_UH-CLUSTER_DESCRIPTION "Scale-out storage cluster  for the UltiHash high-performance object storage")

include(CPack)
