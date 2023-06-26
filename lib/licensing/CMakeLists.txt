cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibLicensing)
message(STATUS "Using ${PROJECT_NAME}...")

if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()

set(LICENSE_SPRING_API_KEY "f43e779a-71bc-460f-af89-69a1c47cbe8b")
set(LICENSE_SPRING_SHARED_KEY "UtVbATx32BYf9QAQtLmcEJ4U5-58SezMIkeyb2Cy8l0")

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/license_spring_credentials.h.in
               ${CMAKE_CURRENT_BINARY_DIR}/license_spring_credentials.h)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

include(ExternalProject)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(LICENSESPRING_VERSION "7.24.0")

if(APPLE)
    set(LICENSESPRING_TARGET "MacUniversal")
elseif (UNIX)
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
        set(LICENSESPRING_TARGET "Linux_AArch64")
    else ()
        set(LICENSESPRING_TARGET "Linux_GCC9.2")
    endif ()
else ()
    message("System not supported for License Spring.")
endif ()

# external dependencies
find_package(Boost 1.80 COMPONENTS system unit_test_framework json REQUIRED)
find_package(OpenSSL 1.1.1 EXACT)
find_package(CURL 4.8.0 REQUIRED)

message("Downloading License Spring to ${CMAKE_BINARY_DIR}/ext")

ExternalProject_Add(
        libLicenseSpringExtern
        PREFIX "${CMAKE_BINARY_DIR}/ext"
        URL "https://kwiki-user-media.s3.amazonaws.com/staging/content/file_widgets/LicenseSpringCppSDKv${LICENSESPRING_VERSION}_${LICENSESPRING_TARGET}.zip"
        DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/ext/download"
        LOG_DOWNLOAD ON
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
        LOG_DIR "${CMAKE_BINARY_DIR}/ext/ep-logs"
        BUILD_BYPRODUCTS ${CMAKE_BINARY_DIR}/ext/src/libLicenseSpringExtern/bin/shared/${CMAKE_BUILD_TYPE}/libLicenseSpring.so
)

file(REMOVE ${CMAKE_BINARY_DIR}/ext/src/libLicenseSpringExtern/bin/shared/${CMAKE_BUILD_TYPE}/libcurl.so.4.8.0)
file(REMOVE ${CMAKE_BINARY_DIR}/ext/src/libLicenseSpringExtern/bin/shared/${CMAKE_BUILD_TYPE}/libcurl.so.4)
file(REMOVE ${CMAKE_BINARY_DIR}/ext/src/libLicenseSpringExtern/bin/shared/${CMAKE_BUILD_TYPE}/libcurl.so)

set(LIB_LICENSE_SPRING_INCLUDE "${CMAKE_BINARY_DIR}/ext/src/libLicenseSpringExtern/include")
file(MAKE_DIRECTORY ${LIB_LICENSE_SPRING_INCLUDE})

add_library(libLicenseSpring SHARED IMPORTED)
set_property(TARGET libLicenseSpring PROPERTY IMPORTED_LOCATION ${CMAKE_BINARY_DIR}/ext/src/libLicenseSpringExtern/bin/shared/${CMAKE_BUILD_TYPE}/libLicenseSpring.so)
target_include_directories(libLicenseSpring INTERFACE ${LIB_LICENSE_SPRING_INCLUDE})
add_dependencies(libLicenseSpring libLicenseSpringExtern)

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS} ${CURL_INCLUDE_DIR})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${CURL_LIBRARIES} ${OPENSSL_LIBRARIES} uhLibIo uhLibLogging libLicenseSpring)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} OBJECT ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PUBLIC ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)
