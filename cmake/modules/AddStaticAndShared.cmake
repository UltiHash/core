# 전역 변수로 PCH 설정
set(PROJECT_PRECOMPILE_HEADER_TARGET
    ""
    CACHE STRING "Project-wide precompiled header")
set(defined_targets "")

function(set_project_precompiled_header_target header)
    set(PROJECT_PRECOMPILE_HEADER_TARGET
        "${header}"
        PARENT_SCOPE)
endfunction()

function(add_static_and_shared library_name)
    # Parse Arguments
    set(options "")
    set(one_value_args SOURCES)
    set(multi_value_args PRIVATE PUBLIC)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}"
                          "${multi_value_args}" ${ARGN})
    if(NOT ARGS_SOURCES)
        message(
            FATAL_ERROR
                "SOURCES argument is required for add_static_and_shared()")
    endif()

    # Object Library
    set(object_library_name "${library_name}_object")
    add_library(${object_library_name} OBJECT ${ARGS_SOURCES})
    target_include_directories(
        ${object_library_name}
        PRIVATE $<TARGET_PROPERTY:${library_name},INCLUDE_DIRECTORIES>)

    if(PROJECT_PRECOMPILE_HEADER_TARGET)
        target_precompile_headers(${object_library_name} REUSE_FROM
                                  ${PROJECT_PRECOMPILE_HEADER_TARGET})
    endif()

    # Static Library
    add_library(${library_name} STATIC $<TARGET_OBJECTS:${object_library_name}>)
    target_link_libraries(
        ${library_name}
        PRIVATE ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})

    set(ARGS_PRIVATE_SHARED "")
    foreach(item IN LISTS ARGS_PRIVATE)
        if(item MATCHES "^uh_")
            list(APPEND ARGS_PRIVATE_SHARED "${item}_shared")
        else()
            list(APPEND ARGS_PRIVATE_SHARED "${item}")
        endif()
    endforeach()

    set(ARGS_PUBLIC_SHARED "")
    foreach(item IN LISTS ARGS_PUBLIC)
        if(item MATCHES "^uh_")
            list(APPEND ARGS_PUBLIC_SHARED "${item}_shared")
        else()
            list(APPEND ARGS_PUBLIC_SHARED "${item}")
        endif()
    endforeach()

    # Shared Library
    set(shared_library_name "${library_name}_shared")
    add_library(${shared_library_name} SHARED
                $<TARGET_OBJECTS:${object_library_name}>)
    target_link_libraries(
        ${shared_library_name}
        PRIVATE ${ARGS_PRIVATE_SHARED}
        PUBLIC ${ARGS_PUBLIC_SHARED})
    list(APPEND defined_targets "${library_name}_shared")

    include(CMakePrintHelpers)
    cmake_print_variables(ARGS_PRIVATE_SHARED ARGS_PUBLIC_SHARED)
    message("here")
endfunction()
