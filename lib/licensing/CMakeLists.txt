
cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibLicensing)
message(STATUS "Using ${PROJECT_NAME}...")

# external dependencies
find_package(Boost 1.80 COMPONENTS system unit_test_framework REQUIRED)
find_package(OpenSSL REQUIRED)

set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} uhLibUtil uhLibLicenseSpring uhLibIo)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LINK_LIBRARIES})

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)

#[===[
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
set(LICENSESPRING_VERSION "7.21.0")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(LICENSESPRING_TARGET "Linux_clang11")
else ()
    set(LICENSESPRING_TARGET "Linux_GCC9.2")
endif ()

ExternalProject_Add(
        LicenceSpring
        PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/ext
        URL "https://kwiki-user-media.s3.amazonaws.com/staging/content/file_widgets/LicenseSpringCppSDKv${LICENSESPRING_VERSION}_${LICENSESPRING_TARGET}.zip"
        DOWNLOAD_EXTRACT_TIMESTAMP true
        LOG_DOWNLOAD ON
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        UPDATE_COMMAND ""
        INSTALL_COMMAND ""
)

# external dependencies
find_package(Boost 1.80 COMPONENTS system unit_test_framework REQUIRED)
find_package(ZLIB REQUIRED)
find_package(CURL 4.8.0 REQUIRED)
find_package(OpenSSL 1.1.1 REQUIRED)


set(${PROJECT_NAME}_INCLUDE_DIRS ${Boost_INCLUDE_DIRS})
set(${PROJECT_NAME}_LINK_LIBRARIES ${Boost_LIBRARIES} ${OPENSSL_LIBRARIES} uhLibUtil uhLibLicenseSpring uhLibIo)

# adding all the .h files of the current project
file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.h)

# adding all the .cpp files of the current project
file(GLOB SOURCE_FILE_LIST ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(${PROJECT_NAME} SHARED ${HEADER_FILE_LIST} ${SOURCE_FILE_LIST})
set(LIBRARY_PATH "src/LicenceSpring/bin/${COMPILE_LIBRARY_NAME_TYPE}/${CMAKE_BUILD_TYPE}")

set_target_properties(${PROJECT_NAME}
        PROPERTIES LINK_FLAGS
        "-L${LIBRARY_PATH} -Wl,--no-as-needed,--enable-new-dtags,-rpath,$ORIGIN/ext/src/LicenceSpring/bin/${COMPILE_LIBRARY_NAME_TYPE}/${CMAKE_BUILD_TYPE},${${PROJECT_NAME}_INCLUDE_DIRS}")
target_link_libraries(${PROJECT_NAME} ${${PROJECT_NAME}_LINK_LIBRARIES} -lLicenseSpring -lssl -lm -lpthread)

# create testing targets
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/testing/CMakeLists.txt)

]===]