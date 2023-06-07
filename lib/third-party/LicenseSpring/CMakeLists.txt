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

#extract OS specific package
if (APPLE)

    if (EXISTS /usr/bin/unzip)
        message("* unzip -- License Spring\n")

        message("Extracting License Spring for Mac from:")
        message("${CMAKE_CURRENT_LIST_DIR}/LicenseSpringCppSDKv7.23.0_MacUniversal.zip")
        message("to build area:")
        message("${CMAKE_CURRENT_BINARY_DIR}\n")

        execute_process(COMMAND
                /usr/bin/unzip -o
                "${CMAKE_CURRENT_LIST_DIR}/LicenseSpringCppSDKv7.23.0_MacUniversal.zip"
                -d "${CMAKE_CURRENT_BINARY_DIR}")
    else ()
        message(FATAL_ERROR "* zip and unzip -- not found")
    endif ()

elseif (UNIX)

    if (EXISTS /usr/bin/unzip)
        message("* unzip -- License Spring\n")

        message("Extracting License Spring for Linux from:")
        message("${CMAKE_CURRENT_LIST_DIR}/LicenseSpringCppSDKv7.23.0_Linux_GCC9.2_IYoup3U.zip")
        message("to build area:")
        message("${CMAKE_CURRENT_BINARY_DIR}\n")

        execute_process(COMMAND
                /usr/bin/unzip -o
                "${CMAKE_CURRENT_LIST_DIR}/LicenseSpringCppSDKv7.23.0_Linux_GCC9.2_IYoup3U.zip"
                -d "${CMAKE_CURRENT_BINARY_DIR}")
    else ()
        message(FATAL_ERROR "* zip and unzip -- not found")
    endif ()

endif ()

message("\n -- unpacking License Spring complete -- \n")

#check if version found
if (LICENSESPRING_FIND_VERSION)
    set(LICENSESPRING_ERROR_MESSAGE "Unable to find libLicenseSpring library v${LICENSESPRING_FIND_VERSION}.")
else ()
    set(LICENSESPRING_ERROR_MESSAGE "Unable to find libLicenseSpring library.")
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