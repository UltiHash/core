# 전역 변수로 PCH 설정
set(PROJECT_PRECOMPILE_HEADER_TARGET
    ""
    CACHE STRING "Project-wide precompiled header")

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
    target_link_libraries(
        ${object_library_name}
        PRIVATE ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})
    set_target_properties(${object_library_name}
                          PROPERTIES POSITION_INDEPENDENT_CODE ON)

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

    # Shared Library
    set(shared_library_name "${library_name}_shared")
    add_library(${shared_library_name} SHARED
                $<TARGET_OBJECTS:${object_library_name}>)
    target_link_libraries(
        ${shared_library_name}
        PRIVATE ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})
endfunction()
