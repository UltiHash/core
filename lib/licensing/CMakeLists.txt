cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibLicensing)
message(STATUS "Using ${PROJECT_NAME}...")

include(ExternalProject)
set(SPRING_SHARED ON)

if (SPRING_SHARED STREQUAL "ON")
    set(COMPILE_LIBRARY_NAME_TYPE "shared")
else ()
    set(COMPILE_LIBRARY_NAME_TYPE "static")
endif ()

set(CMAKE_VERBOSE_MAKEFILE ON)
set(LICENSESPRING_VERSION "7.24.0")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(LICENSESPRING_TARGET "Linux_clang11")
else ()
    set(LICENSESPRING_TARGET "Linux_GCC9.2")
endif ()

# external dependencies
find_package(Boost 1.80 COMPONENTS system unit_test_framework json REQUIRED)
find_package(OpenSSL 1.1.1 REQUIRED)
find_package(CURL 4.8.0 REQUIRED)

message("Downloading License Spring to ${CMAKE_BINARY_DIR}/ext")

ExternalProject_Add(
        libLicenseSpring
        PREFIX "${CMAKE_BINARY_DIR}/ext"
        URL "https://kwiki-user-media.s3.amazonaws.com/staging/content/file_widgets/LicenseSpringCppSDKv${LICENSESPRING_VERSION}_${LICENSESPRING_TARGET}.zip"
        DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/ext/download"
        DOWNLOAD_EXTRACT_TIMESTAMP true
        LOG_DOWNLOAD ON
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DIR "${CMAKE_BINARY_DIR}/ext/ep-logs"
)


set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} uhLibIo uhLibLogging)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS} "${CMAKE_BINARY_DIR}/ext/src/libLicenseSpring/include/LicenseSpring/*")

set(LIBRARY_PATH "ext/src/LicenceSpring/bin/${COMPILE_LIBRARY_NAME_TYPE}/${CMAKE_BUILD_TYPE}")

set_target_properties(${PROJECT_NAME}
        PROPERTIES LINK_FLAGS
        "-L${LIBRARY_PATH} -Wl,--no-as-needed,--enable-new-dtags,-rpath,$ORIGIN/ext/src/LicenceSpring/bin/${COMPILE_LIBRARY_NAME_TYPE}/${CMAKE_BUILD_TYPE}")

add_custom_target(download_ls)
add_dependencies(${PROJECT_NAME} libLicenseSpring)
target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LINK_LIBRARIES} "${CMAKE_BINARY_DIR}/ext/src/libLicenseSpring/${SPRING_SHARED}/${CMAKE_BUILD_TYPE}/libLicenseSpring.so")

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)
