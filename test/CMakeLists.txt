file(DOWNLOAD 
    https://raw.githubusercontent.com/yhirose/cpp-httplib/v0.18.5/httplib.h
    ${CMAKE_BINARY_DIR}/test/httplib.h
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

    set(target_name "${PROJECT_NAME}_${name}")

    add_executable(${target_name} ${name}.cpp)
    # target_compile_options(${target_name} PRIVATE -fno-inline)
    # target_compile_definitions( ${target_name} PRIVATE
    # BOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)

    target_link_libraries(
        ${target_name}
        PRIVATE ${PROJECT_NAME}_shared Boost::unit_test_framework uh_test_util
                uh_test_config ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})

    target_precompile_headers(${target_name} REUSE_FROM uh_precompile_headers)
    add_test(NAME ${target_name} COMMAND ${target_name})
    get_property(LABELS GLOBAL PROPERTY TEST_TYPE)
    set_tests_properties(${target_name} PROPERTIES LABELS ${LABELS})
endfunction()

add_subdirectory(unit)
add_subdirectory(integration)
