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

set(SPRING_SHARED ON)

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

#remove either shared or static libraries from working directory as well as not needed content
file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/samples")
file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/documentation")

if(${SPRING_SHARED} STREQUAL "ON")
    file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/static")

    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/shared/Release")
    else ()
        file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/shared/Debug")
    endif ()
else ()
    file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/shared")

    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/static/Release")
    else ()
        file(REMOVE_RECURSE "${LICENSE_SPRING_SOURCE_FOLDER}/bin/static/Debug")
    endif ()
endif ()

#[===[

if(APPLE)
    # On macOS, we need the macLICENSESPRING package
    find_library(LICENSESPRING_LIBRARY libLicenseSpring)
    find_path(LICENSESPRING_INCLUDE_DIR libLicenseSpring.so)
    if(LICENSESPRING_LIBRARY AND LICENSESPRING_INCLUDE_DIR)
        set(LICENSESPRING_FOUND TRUE)
        set(LICENSESPRING_INCLUDE_DIRS ${LICENSESPRING_INCLUDE_DIR})
        set(LICENSESPRING_LIBRARIES ${LICENSESPRING_LIBRARY})
        execute_process(COMMAND ${LICENSESPRING_LIBRARY} --version OUTPUT_VARIABLE LICENSESPRING_FOUND_VERSION)
        string(REGEX REPLACE ".*([0-9]+\\.[0-9]+).*" "\\1" LICENSESPRING_FOUND_VERSION ${LICENSESPRING_FOUND_VERSION})
    endif()
elseif(UNIX)
    # On Linux, we can use the regular find_library and find_path calls to find libLICENSESPRING
    find_library(LICENSESPRING_LIBRARY libLicenseSpring)
    find_path(LICENSESPRING_INCLUDE_DIR fuse.h)
    if(LICENSESPRING_LIBRARY AND LICENSESPRING_INCLUDE_DIR)
        set(LICENSESPRING_FOUND TRUE)
        set(LICENSESPRING_INCLUDE_DIRS ${LICENSESPRING_INCLUDE_DIR})
        set(LICENSESPRING_LIBRARIES ${LICENSESPRING_LIBRARY})
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            execute_process(COMMAND pkg-config --modversion fuse OUTPUT_VARIABLE LICENSESPRING_FOUND_VERSION)
            string(REGEX REPLACE "\\.([0-9]+)\\.[0-9]+" ".\\1" LICENSESPRING_FOUND_VERSION ${LICENSESPRING_FOUND_VERSION})
            string(STRIP "${LICENSESPRING_FOUND_VERSION}" LICENSESPRING_FOUND_VERSION)
        endif()
    endif()
endif()

# check found version
if(LICENSESPRING_FIND_VERSION AND LICENSESPRING_FOUND)
    set(LICENSESPRING_FOUND_VERSION "${LICENSESPRING_FOUND_VERSION}")

    if(LICENSESPRING_FIND_VERSION_EXACT)
        if(NOT ${LICENSESPRING_FOUND_VERSION} VERSION_EQUAL ${LICENSESPRING_FIND_VERSION})
            set(LICENSESPRING_FOUND FALSE)
        endif()
    else()
        if(${LICENSESPRING_FOUND_VERSION} VERSION_LESS ${LICENSESPRING_FIND_VERSION})
            set(LICENSESPRING_FOUND FALSE)
        endif()
    endif()

    if(NOT LICENSESPRING_FOUND)
        set(LICENSESPRING_ERROR_MESSAGE "Unable to find libLICENSESPRING v${LICENSESPRING_FIND_VERSION} (${LICENSESPRING_FOUND_VERSION} was found).")
    endif()
endif (LICENSESPRING_FIND_VERSION AND LICENSESPRING_FOUND)

# report result
if(LICENSESPRING_FOUND)
    if(NOT LICENSESPRING_FIND_QUIETLY)
        message(STATUS "Found libLICENSESPRING v${LICENSESPRING_FOUND_VERSION}.")
    endif(NOT LICENSESPRING_FIND_QUIETLY)

    mark_as_advanced(LICENSESPRING_INCLUDE_DIRS LICENSESPRING_LIBRARIES)
else()
    if(LICENSESPRING_FIND_REQUIRED)
        message(SEND_ERROR "${LICENSESPRING_ERROR_MESSAGE}.")
    endif()
endif()

]===]