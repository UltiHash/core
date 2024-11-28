configure_file(test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.h)

function(uh_add_test name)
    set(options "")
    set(one_value_args SOURCES)
    set(multi_value_args PRIVATE PUBLIC)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}"
                          "${multi_value_args}" ${ARGN})

    set(target_name "${PROJECT_NAME}_${name}")

    add_executable(${target_name} ${name}.cpp)
    target_compile_definitions(
        ${target_name} PRIVATE BOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)
    target_link_libraries(
        ${target_name}
        PRIVATE uh_test uh_debug Boost::unit_test_framework ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})
    target_include_directories(${target_name}
                               PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_precompile_headers(${target_name} REUSE_FROM uh_precompile_headers)
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

add_library(gdv_fixture INTERFACE)
target_link_libraries(gdv_fixture INTERFACE uh_recovery uh_storage uh_utils
                                            uh_global_data)

uh_add_test(test_address PRIVATE uh_types)
uh_add_test(test_big_int PRIVATE uh_types)
uh_add_test(test_scoped_buffer PRIVATE uh_types)

uh_add_test(test_awaitable_promise PRIVATE uh_coroutines)
uh_add_test(test_promise_future PRIVATE uh_coroutines)

uh_add_test(test_data_store PRIVATE uh_storage)
uh_add_test(test_reference_counter PRIVATE uh_storage)

uh_add_test(test_ec PRIVATE uh_ec uh_telemetry)

uh_add_test(test_fragment_set PRIVATE uh_dedupe_set gdv_fixture)
uh_add_test(test_serialization PRIVATE uh_dedupe_set)

uh_add_test(test_global_data_view PRIVATE uh_utils gdv_fixture)
uh_add_test(test_pointer_traits PRIVATE uh_utils)
uh_add_test(test_pool PRIVATE uh_utils)
uh_add_test(test_strings PRIVATE uh_utils)
uh_add_test(test_xml_parser PRIVATE uh_utils)

uh_add_test(test_hash PRIVATE uh_crypto)

uh_add_test(test_http_range PRIVATE uh_http)

uh_add_test(test_license PRIVATE uh_license)

uh_add_test(test_lfu_cache PRIVATE uh_caches)
uh_add_test(test_lru_cache PRIVATE uh_caches)
uh_add_test(test_plain_cache PRIVATE uh_caches)

uh_add_test(test_formats PRIVATE uh_entrypoint)

uh_add_test(test_service_registry PRIVATE uh_registry)
uh_add_test(test_services PRIVATE uh_registry uh_service_discovery)

add_library(uh_mock_entrypoint SHARED entrypoint.cpp)
target_link_libraries(uh_mock_entrypoint PRIVATE uh_commands uh_policy)

uh_add_test(test_policies PRIVATE uh_utils uh_mock_entrypoint nlohmann_json)
uh_add_test(test_policies_with_action PRIVATE uh_policy uh_mock_entrypoint
            nlohmann_json)
uh_add_test(test_policies_with_condition PRIVATE uh_policy uh_mock_entrypoint
            nlohmann_json)
uh_add_test(test_policies_with_examples PRIVATE uh_policy uh_mock_entrypoint
            nlohmann_json)
# uh_add_test(test_variables uh_mock_entrypoint)
