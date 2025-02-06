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
# - SETTING: list of interfaces, which are used to build the library
# - INTERFACE: list of interfaces, which should also be propagated to the dependents
# - PRIVATE: list of libraries, which should be linked privately
# - PUBLIC: list of libraries, which should be linked publicly
# - PRIVATE_BUNDLE: list of library bundles, which should be linked privately 
# - PUBLIC_BUNDLE: list of library bundles, which should be linked publicly
function(add_library_bundle name)
    cmake_parse_arguments(
        ARGS "" ""
        "SOURCES;SETTING;INTERFACE;PRIVATE;PUBLIC;PRIVATE_BUNDLE;PUBLIC_BUNDLE" ${ARGN})

    add_library(${name}_object OBJECT ${ARGS_SOURCES})
    target_link_libraries(
        ${name}_object
        PRIVATE ${ARGS_INTERFACE} ${ADDITIONAL_INTERFACES} ${ARGS_PRIVATE}
                ${ARGS_SETTING} ${ARGS_PRIVATE_BUNDLE}
        PUBLIC ${ARGS_PUBLIC} ${ARGS_PUBLIC_BUNDLE}
        INTERFACE ${ARGS_INTERFACE} ${ADDITIONAL_INTERFACES})

    add_library(${name} STATIC $<TARGET_OBJECTS:${name}_object>)
    target_link_libraries(
        ${name}
        PRIVATE ${ARGS_INTERFACE} ${ADDITIONAL_INTERFACES} ${ARGS_PRIVATE}
                ${ARGS_SETTING} ${ARGS_PRIVATE_BUNDLE}
        PUBLIC ${ARGS_PUBLIC} ${ARGS_PUBLIC_BUNDLE}
        INTERFACE ${ARGS_INTERFACE} ${ADDITIONAL_INTERFACES})

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
        PRIVATE ${ARGS_INTERFACE} ${ADDITIONAL_INTERFACES} ${private_static}
                ${ARGS_SETTING}
        PUBLIC ${public_static} ${public_bundle_shared} 
               # Exceptional visibility: every shared library should be propagated
               ${private_bundle_shared} 
        INTERFACE ${ARGS_INTERFACE} ${ADDITIONAL_INTERFACES})
endfunction()
