file(DOWNLOAD
    https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.5/httplib.h
    ${CMAKE_CURRENT_BINARY_DIR}/httplib.h
)
file(DOWNLOAD
    https://github.com/eranpeer/FakeIt/raw/refs/tags/2.4.1/single_header/standalone/fakeit.hpp
    ${CMAKE_CURRENT_BINARY_DIR}/fakeit.hpp
)

add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE ${CMAKE_BINARY_DIR}/test)

add_subdirectory(lib)
add_subdirectory(performance)

configure_file(test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.h @ONLY)
add_library(uh_test_config INTERFACE)
target_include_directories(uh_test_config INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(uh_test_config INTERFACE magic_enum)

function(uh_add_test name)
    # Parse Arguments
    set(options "")
    set(one_value_args MODE)
    set(multi_value_args PRIVATE PUBLIC LABELS)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}"
                          "${multi_value_args}" ${ARGN})

    set(target_name "${name}")
    if(ARGS_MODE)
        set(target_name "${target_name}_${ARGS_MODE}")
    endif()
    add_executable(${target_name} ${name}.cpp)
    if(ARGS_MODE)
        string(TOUPPER "${ARGS_MODE}" MODE_STRING)
        target_compile_definitions(${target_name} PRIVATE "-D${MODE_STRING}")
    endif()
    target_link_libraries(${target_name}
        PRIVATE Boost::unit_test_framework uh_test_util
                uh_test_config ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})

    target_precompile_headers(${target_name} REUSE_FROM uh_precompile_headers)

    # execute_process(
    #     COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${target_name} --list_content
    #     ERROR_VARIABLE TEST_LIST
    #     OUTPUT_STRIP_TRAILING_WHITESPACE
    # )

    # boost test exec option: --log_level=all --run_test=multiple*/determine*

    add_test(NAME ${target_name} COMMAND ${target_name})
    get_property(LABELS GLOBAL PROPERTY TEST_TYPE)
    set_tests_properties(${target_name} PROPERTIES LABELS ${LABELS})
endfunction()

add_subdirectory(unit)
add_subdirectory(integration)
