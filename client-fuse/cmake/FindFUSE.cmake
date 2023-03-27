# Find libFUSE
#
# This module defines the following variables:
#
# FUSE_FOUND          - True if libFUSE was found
# FUSE_INCLUDE_DIRS   - Include directories for libFUSE
# FUSE_LIBRARIES      - Libraries needed to use libFUSE
# FUSE_FOUND_VERSION - Version string for libFUSE
#
# Note that on macOS, this module requires the "macFUSE" package to be installed.

if(FUSE_FIND_VERSION)
    set(FUSE_ERROR_MESSAGE "Unable to find libFUSE library v${FUSE_FIND_VERSION}.")
else()
    set(FUSE_ERROR_MESSAGE "Unable to find libFUSE library.")
endif()


if(APPLE)
    # On macOS, we need the macFUSE package
    #find_library(FUSE_LIBRARY fuse)
    #find_path(FUSE_INCLUDE_DIR fuse.h)
    #if(FUSE_LIBRARY AND FUSE_INCLUDE_DIR)
    #    set(FUSE_FOUND TRUE)
    #    set(FUSE_INCLUDE_DIRS ${FUSE_INCLUDE_DIR})
    #    set(FUSE_LIBRARIES ${FUSE_LIBRARY})
    #    execute_process(COMMAND ${FUSE_LIBRARY} --version OUTPUT_VARIABLE FUSE_FOUND_VERSION)
    #    string(REGEX REPLACE ".*([0-9]+\\.[0-9]+).*" "\\1" FUSE_FOUND_VERSION ${FUSE_FOUND_VERSION})
    #endif()
elseif(UNIX)
    # On Linux, we can use the regular find_library and find_path calls to find libFUSE
    find_library(FUSE_LIBRARY fuse)
    find_path(FUSE_INCLUDE_DIR fuse.h)
    if(FUSE_LIBRARY AND FUSE_INCLUDE_DIR)
        set(FUSE_FOUND TRUE)
        set(FUSE_INCLUDE_DIRS ${FUSE_INCLUDE_DIR})
        set(FUSE_LIBRARIES ${FUSE_LIBRARY})
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            execute_process(COMMAND pkg-config --modversion fuse OUTPUT_VARIABLE FUSE_FOUND_VERSION)
            string(REGEX REPLACE "\\.([0-9]+)\\.[0-9]+" ".\\1" FUSE_FOUND_VERSION ${FUSE_FOUND_VERSION})
            string(STRIP "${FUSE_FOUND_VERSION}" FUSE_FOUND_VERSION)
        endif()
    endif()
endif()

# check found version
if(FUSE_FIND_VERSION AND FUSE_FOUND)
    set(FUSE_FOUND_VERSION "${FUSE_FOUND_VERSION}")

    if(FUSE_FIND_VERSION_EXACT)
        if(NOT ${FUSE_FOUND_VERSION} VERSION_EQUAL ${FUSE_FIND_VERSION})
            set(FUSE_FOUND FALSE)
        endif()
    else()
        if(${FUSE_FOUND_VERSION} VERSION_LESS ${FUSE_FIND_VERSION})
            set(FUSE_FOUND FALSE)
        endif()
    endif()

    if(NOT FUSE_FOUND)
        set(FUSE_ERROR_MESSAGE "Unable to find libFUSE v${FUSE_FIND_VERSION} (${FUSE_FOUND_VERSION} was found).")
    endif()
endif (FUSE_FIND_VERSION AND FUSE_FOUND)

# report result
if(FUSE_FOUND)
    if(NOT FUSE_FIND_QUIETLY)
        message(STATUS "Found libFUSE v${FUSE_FOUND_VERSION}.")
    endif(NOT FUSE_FIND_QUIETLY)

    mark_as_advanced(FUSE_INCLUDE_DIRS FUSE_LIBRARIES)
else()
    if(FUSE_FIND_REQUIRED)
        message(SEND_ERROR "${FUSE_ERROR_MESSAGE}.")
    endif()
endif()