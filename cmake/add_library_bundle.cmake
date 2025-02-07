set(PROJECT_SETTING_INTERFACES "")

function(register_additional_interface target)
    list(APPEND ADDITIONAL_INTERFACES ${target})
    set(ADDITIONAL_INTERFACES
        ${ADDITIONAL_INTERFACES}
        PARENT_SCOPE)
endfunction()

# Declare object/static/shared library
# Usage:
# - SOURCES: list of source files
# - PRIVATE: list of libraries, which should be linked privately
# - PUBLIC: list of libraries, which should be linked publicly
# - PRIVATE_BUNDLE: list of library bundles, which should be linked privately
# - PUBLIC_BUNDLE: list of library bundles, which should be linked publicly
function(add_library_bundle name)
    cmake_parse_arguments(
        ARGS "" ""
        "SOURCES;PRIVATE;PUBLIC;PRIVATE_BUNDLE;PUBLIC_BUNDLE" ${ARGN})

    add_library(${name}_object OBJECT ${ARGS_SOURCES})
    target_link_libraries(
        ${name}_object
        PRIVATE ${ARGS_PRIVATE} ${ARGS_PRIVATE_BUNDLE}
        PUBLIC ${ADDITIONAL_INTERFACES} ${ARGS_PUBLIC} ${ARGS_PUBLIC_BUNDLE})

    add_library(${name} STATIC $<TARGET_OBJECTS:${name}_object>)
    target_link_libraries(
        ${name}
        PRIVATE ${ARGS_PRIVATE} ${ARGS_PRIVATE_BUNDLE}
        PUBLIC ${ADDITIONAL_INTERFACES} ${ARGS_PUBLIC} ${ARGS_PUBLIC_BUNDLE})

    set(private_static)
    set(public_static)
    foreach(lib ${ARGS_PRIVATE})
        list(APPEND private_static $<LINK_LIBRARY:WHOLE_ARCHIVE,${lib}>)
    endforeach()

    foreach(lib ${ARGS_PUBLIC})
        list(APPEND public_static $<LINK_LIBRARY:WHOLE_ARCHIVE,${lib}>)
    endforeach()

    set(private_bundle_shared "")
    set(public_bundle_shared "")
    foreach(dep IN LISTS ARGS_PRIVATE_BUNDLE)
        list(APPEND private_bundle_shared "${dep}_shared")
    endforeach()
    foreach(dep IN LISTS ARGS_PUBLIC_BUNDLE)
        list(APPEND public_bundle_shared "${dep}_shared")
    endforeach()

    add_library(${name}_shared SHARED $<TARGET_OBJECTS:${name}_object>)
    target_link_libraries(
        ${name}_shared
        PRIVATE ${ARGS_PRIVATE} ${ARGS_PRIVATE_BUNDLE}
        PUBLIC ${ADDITIONAL_INTERFACES} ${ARGS_PUBLIC} ${ARGS_PUBLIC_BUNDLE})

    # We don't like to have external shared library as our dependency
    foreach(dep ${PUBLIC})
        get_target_property(DEP_LIBS ${dep} INTERFACE_LINK_LIBRARIES)
        if (DEP_LIBS)
            message(FATAL_ERROR "We have shared library as our dependency: ${DEP_LIBS}")
        endif()
    endforeach()
    foreach(dep ${PRIVATE})
        if (DEP_LIBS)
            message(FATAL_ERROR "We have shared library as our dependency: ${DEP_LIBS}")
        endif()
    endforeach()

    # Expose shared libraries, even though they are PRIVATE
    foreach(dep ${private_bundle_shared})
        get_target_property(DEP_LIBS ${dep} INTERFACE_LINK_LIBRARIES)
        foreach(lib ${DEP_LIBS})
            target_link_libraries(${name}_shared PUBLIC $<IF:$<TARGET_PROPERTY:${lib},TYPE>,SHARED_LIBRARY,${lib},>)
        endforeach()
    endforeach()

endfunction()
