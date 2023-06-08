# Find libLICENSESPRING
#
# This module defines the following variables:
#
# LICENSESPRING_FOUND          - True if libLICENSESPRING was found
# LICENSESPRING_INCLUDE_DIRS   - Include directories for libLICENSESPRING
# LICENSESPRING_LIBRARIES      - Libraries needed to use libLICENSESPRING
# LICENSESPRING_FOUND_VERSION - Version string for libLICENSESPRING
#
# Note that on macOS, this module requires the "macLICENSESPRING" package to be installed.

cmake_minimum_required(VERSION 3.22.0 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 20)

project(uhLibLicenseSpring)
message(STATUS "Using ${PROJECT_NAME}...")

set(SPRING_SHARED ON)

find_package(ZLIB REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenSSL REQUIRED)

#extract OS specific package

MACRO(SUBPATHWORKPATHLICENSE result curdir checkmatch)
    FILE(GLOB children ${curdir}/*) # This was changed
    SET(dirlist "")
    FOREACH(child ${children})
        IF(${child} MATCHES ${checkmatch}) # This was changed
            LIST(APPEND dirlist ${child})
        ENDIF()
    ENDFOREACH()
    SET(${result} ${dirlist})
ENDMACRO()

if (APPLE)

    if (EXISTS /usr/bin/unzip)
        message("* unzip -- License Spring\n")

        message("Extracting License Spring for Mac from:")
        SUBPATHWORKPATHLICENSE(MACZIP ${CMAKE_CURRENT_LIST_DIR} "/LicenseSpring/LicenseSpringCppSDK[A-Za-z0-9_.]*Mac")
        message("${MACZIP}")
        message("to build area:")
        message("${CMAKE_CURRENT_BINARY_DIR}\n")

        if("${MACZIP}" STREQUAL "")
            message(FATAL_ERROR "* libLicenseSpring.zip -- not found")
        endif ()

        execute_process(COMMAND
                /usr/bin/unzip -o
                "${MACZIP}"
                -d "${CMAKE_CURRENT_BINARY_DIR}")
    else ()
        message(FATAL_ERROR "* zip and unzip -- not found")
    endif ()

elseif (UNIX)

    if (EXISTS /usr/bin/unzip)
        message("* unzip -- License Spring\n")

        message("Extracting License Spring for Linux from:")
        SUBPATHWORKPATHLICENSE(LINUXZIP ${CMAKE_CURRENT_LIST_DIR} "/LicenseSpring/LicenseSpringCppSDK[A-Za-z0-9_.]*Linux")
        message("${LINUXZIP}")
        message("to build area:")
        message("${CMAKE_CURRENT_BINARY_DIR}\n")

        if("${LINUXZIP}" STREQUAL "")
            message(FATAL_ERROR "* libLicenseSpring.zip -- not found")
        endif ()

        execute_process(COMMAND
                /usr/bin/unzip -o
                "${LINUXZIP}"
                -d "${CMAKE_CURRENT_BINARY_DIR}")
    else ()
        message(FATAL_ERROR "* zip and unzip -- not found")
    endif ()

endif ()

set(LICENSESPRING_FOUND True)

message("\n -- unpacking License Spring complete -- \n")

#read version of license spring
SUBPATHWORKPATHLICENSE(LICENSE_SPRING_SOURCE_FOLDER ${CMAKE_CURRENT_BINARY_DIR} "/LicenseSpring/LicenseSpring*")

message("${LICENSE_SPRING_SOURCE_FOLDER}\n")

file(READ "${LICENSE_SPRING_SOURCE_FOLDER}/README" spring_reader)
string(REPLACE "\n" ";" spring_reader "${spring_reader}")
foreach(line IN LISTS spring_reader)
    set(FIRST_README_VERSION_LINE ${line})
    BREAK()
endforeach()

string(REGEX REPLACE "[A-Za-z+]" "" LICENSESPRING_FOUND_VERSION ${FIRST_README_VERSION_LINE})

message("License Spring version:")
message("${LICENSESPRING_FOUND_VERSION}\n")
message("Loading License Spring in mode: ${CMAKE_BUILD_TYPE}\n")

set(LICENSESPRING_INCLUDE_DIRS "${LICENSE_SPRING_SOURCE_FOLDER}/include/LicenseSpring")

#remove either shared or static libraries from working directory as well as not needed content
file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/samples")
file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/documentation")

#copy and overwrite files
set(COPY_SOURCE_FOLDER "${CMAKE_CURRENT_BINARY_DIR}")

file(MAKE_DIRECTORY "${COPY_SOURCE_FOLDER}/include")
file(COPY "${LICENSE_SPRING_SOURCE_FOLDER}/include/" DESTINATION "${COPY_SOURCE_FOLDER}/include/")

if(${SPRING_SHARED} STREQUAL "ON")
    file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/static")

    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/shared/Release")

        file(COPY "${LICENSE_SPRING_SOURCE_FOLDER}/bin/shared/Debug/" DESTINATION "${COPY_SOURCE_FOLDER}/bin")
    else ()
        file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/shared/Debug")

        file(COPY "${LICENSE_SPRING_SOURCE_FOLDER}/bin/shared/Release/" DESTINATION "${COPY_SOURCE_FOLDER}/bin")
    endif ()

    set(LICENSESPRING_LIBRARY "${COPY_SOURCE_FOLDER}/bin/libLicenseSpring.so")
    set(LICENSESPRING_LIBRARIES ${LICENSESPRING_LIBRARY})
    message("Using library: ${LICENSESPRING_LIBRARIES}")

    #add_library(libLicenseSpring INTERFACE "${LICENSESPRING_INCLUDE_DIRS}")
    #target_include_directories(libLicenseSpring INTERFACE "${LICENSESPRING_INCLUDE_DIRS}")

else ()
    file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/shared")

    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/static/Release")

        file(COPY "${LICENSE_SPRING_SOURCE_FOLDER}/bin/static/Debug/" DESTINATION "${COPY_SOURCE_FOLDER}/bin")
    else ()
        file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/static/Debug")

        file(COPY "${LICENSE_SPRING_SOURCE_FOLDER}/bin/static/Release/" DESTINATION "${COPY_SOURCE_FOLDER}/bin")
    endif ()

    set(LICENSESPRING_LIBRARY "${COPY_SOURCE_FOLDER}/bin/libLicenseSpring.a")
    set(LICENSESPRING_LIBRARIES ${LICENSESPRING_LIBRARY})
    message("Using library: ${LICENSESPRING_LIBRARIES}")

    #add_library(libLicenseSpring INTERFACE "${LICENSESPRING_INCLUDE_DIRS}")
    #target_include_directories(libLicenseSpring INTERFACE "${LICENSESPRING_INCLUDE_DIRS}")
endif ()

message("Included License Spring at: ${LICENSESPRING_INCLUDE_DIRS}")

# adding all the .h files of the current project

file(COPY "${COPY_SOURCE_FOLDER}/include" DESTINATION "${CMAKE_CURRENT_LIST_DIR}")
file(COPY "${COPY_SOURCE_FOLDER}/bin/" DESTINATION "${CMAKE_CURRENT_LIST_DIR}/bin_${CMAKE_BUILD_TYPE}/")

file(GLOB HEADER_FILE_LIST ${CMAKE_CURRENT_BINARY_DIR}/include/*.h)

#include_directories(${CMAKE_CURRENT_LIST_DIR}/include/)

set(${PROJECT_NAME}_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/include)
set(${PROJECT_NAME}_LINK_LIBRARIES ${OPENSSL_LIBRARIES} ${CURL_LIBRARIES} ${ZLIB_LIBRARIES} ${PROJECT_NAME})

if(${SPRING_SHARED} STREQUAL "ON")
    add_library(${PROJECT_NAME} ${HEADER_FILE_LIST}#[===[${SOURCE_FILE_LIST}]===] IMPORTED SHARED)
    set_property(TARGET ${PROJECT_NAME} PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/bin/libLicenseSpring.so")
else ()
    add_library(${PROJECT_NAME} ${HEADER_FILE_LIST}#[===[${SOURCE_FILE_LIST}]===] IMPORTED STATIC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/bin/libLicenseSpring.a")
endif ()

message("Link to: ${${PROJECT_NAME}_LINK_LIBRARIES}")
message("Include directories to ")

target_link_libraries(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_LINK_LIBRARIES})
target_include_directories(${PROJECT_NAME} INTERFACE ${${PROJECT_NAME}_INCLUDE_DIRS})

