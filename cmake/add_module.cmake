set(PROJECT_SETTING_INTERFACES "")

function(register_additional_interface target)
    list(APPEND ADDITIONAL_INTERFACES ${target})
    set(ADDITIONAL_INTERFACES
        ${ADDITIONAL_INTERFACES}
        PARENT_SCOPE)
endfunction()

function(add_module name)
    cmake_parse_arguments(
        ARG "" "SOURCES"
        "SETTING;INTERFACE;PRIVATE;PUBLIC;PRIVATE_MODULE;PUBLIC_MODULE" ${ARGN})

    add_library(${name}_object OBJECT ${ARG_SOURCES})
    target_link_libraries(
        ${name}_object
        PRIVATE ${ARG_INTERFACE} ${ADDITIONAL_INTERFACES} ${ARG_PRIVATE}
                ${ARG_SETTING} ${ARG_PRIVATE_MODULE}
        PUBLIC ${ARG_PUBLIC} ${ARG_PUBLIC_MODULE}
        INTERFACE ${ARG_INTERFACE} ${ADDITIONAL_INTERFACES})

    add_library(${name} STATIC $<TARGET_OBJECTS:${name}_object>)
    target_link_libraries(
        ${name}
        PRIVATE ${ARG_INTERFACE} ${ADDITIONAL_INTERFACES} ${ARG_PRIVATE}
                ${ARG_SETTING} ${ARG_PRIVATE_MODULE}
        PUBLIC ${ARG_PUBLIC} ${ARG_PUBLIC_MODULE}
        INTERFACE ${ARG_INTERFACE} ${ADDITIONAL_INTERFACES})

    set(private_static)
    set(public_static)
    foreach(lib ${ARG_PRIVATE})
        list(APPEND private_static $<LINK_LIBRARY:WHOLE_ARCHIVE,${lib}>)
    endforeach()

    foreach(lib ${ARG_PUBLIC})
        list(APPEND public_static $<LINK_LIBRARY:WHOLE_ARCHIVE,${lib}>)
    endforeach()

    set(private_module_shared "")
    set(public_module_shared "")
    foreach(dep IN LISTS ARG_PRIVATE_MODULE)
        list(APPEND private_module_shared "${dep}_shared")
    endforeach()
    foreach(dep IN LISTS ARG_PUBLIC_MODULE)
        list(APPEND public_module_shared "${dep}_shared")
    endforeach()

    add_library(${name}_shared SHARED $<TARGET_OBJECTS:${name}_object>)
    target_link_libraries(
        ${name}_shared
        PRIVATE ${ARG_INTERFACE} ${ADDITIONAL_INTERFACES} ${private_static}
                ${ARG_SETTING} ${private_module_shared}
        PUBLIC ${public_static} ${public_module_shared}
        INTERFACE ${ARG_INTERFACE} ${ADDITIONAL_INTERFACES})
endfunction()
