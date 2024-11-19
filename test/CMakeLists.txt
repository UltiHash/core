configure_file(test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.h)

function(uh_add_test name)
    set(target_name "${PROJECT_NAME}_${name}")

    add_executable(${target_name} ${name}.cpp)
    target_compile_options(
        ${target_name} PRIVATE -fno-inline
                               -DBOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)

    if(NOT DEFINED FIRST_TEST_TARGET)
        target_precompile_headers(${target_name} PRIVATE <boost/asio.hpp>)
        set_property(GLOBAL PROPERTY FIRST_TEST_TARGET ${target_name})
    else()
        get_property(first_target GLOBAL PROPERTY FIRST_TEST_TARGET)
        target_precompile_headers(${target_name} REUSE_FROM ${first_target})
    endif()

    target_link_libraries(${target_name} uh_test uh_debug
                          Boost::unit_test_framework ${ARGN})
    target_include_directories(${target_name}
                               PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

uh_add_test(test_address uh_types)
uh_add_test(test_awaitable_promise uh_coroutines)
uh_add_test(test_big_int uh_types)
uh_add_test(test_data_store uh_storage)
uh_add_test(test_ec uh_ec uh_telemetry)
uh_add_test(test_fragment_set uh_dedupe_set)
uh_add_test(test_global_data_view uh_utils)
uh_add_test(test_hash uh_crypto)
uh_add_test(test_http_range uh_http)
uh_add_test(test_lfu_cache uh_caches)
uh_add_test(test_license uh_license)
uh_add_test(test_lru_cache uh_caches)
uh_add_test(test_plain_cache uh_caches)
uh_add_test(test_pointer_traits uh_utils)
uh_add_test(test_formats uh_entrypoint)
uh_add_test(test_pool uh_utils)
uh_add_test(test_promise_future uh_coroutines)
uh_add_test(test_reference_counter uh_storage)
uh_add_test(test_scoped_buffer uh_types)
uh_add_test(test_serialization uh_dedupe_set)
uh_add_test(test_service_registry uh_registry)
uh_add_test(test_services uh_registry uh_service_discovery)
uh_add_test(test_strings uh_utils)
uh_add_test(test_xml_parser uh_utils)

add_library(uh_mock_entrypoint entrypoint.cpp)
target_link_libraries(uh_mock_entrypoint uh_commands uh_policy)

uh_add_test(test_policies uh_utils uh_mock_entrypoint nlohmann_json)
uh_add_test(test_policies_with_action uh_policy uh_mock_entrypoint
            nlohmann_json)
uh_add_test(test_policies_with_condition uh_policy uh_mock_entrypoint
            nlohmann_json)
uh_add_test(test_policies_with_examples uh_policy uh_mock_entrypoint
            nlohmann_json)
# uh_add_test(test_variables uh_mock_entrypoint)
