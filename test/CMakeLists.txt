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

function(uh_add_test name)
    # Parse Arguments
    set(options "")
    set(multi_value_args PRIVATE PUBLIC LABELS)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}"
                          "${multi_value_args}" ${ARGN})

    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name}
        PRIVATE Boost::unit_test_framework uh_test_util
                uh_test_config ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})

    target_precompile_headers(${name} REUSE_FROM uh_precompile_headers)
    add_test(NAME ${name} COMMAND ${name})
    get_property(LABELS GLOBAL PROPERTY TEST_TYPE)
    set_tests_properties(${name} PROPERTIES LABELS ${LABELS})
endfunction()

add_subdirectory(unit)
add_subdirectory(integration)
