configure_file(test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.h)

add_library(uh_mockup_entrypoint SHARED gdv_fixture.h entrypoint.cpp)
target_link_libraries(uh_mockup_entrypoint PUBLIC ${PROJECT_NAME}_shared)
target_precompile_headers(uh_mockup_entrypoint REUSE_FROM uh_precompile_headers)

function(uh_add_test name)
    # Parse Arguments
    set(options "")
    set(multi_value_args PRIVATE PUBLIC)
    cmake_parse_arguments(ARGS "${options}" "${one_value_args}"
                          "${multi_value_args}" ${ARGN})

    set(target_name "${PROJECT_NAME}_${name}")

    add_executable(${target_name} ${name}.cpp)
    # target_compile_options(${target_name} PRIVATE -fno-inline)
    # target_compile_definitions( ${target_name} PRIVATE
    # BOOST_ASIO_DISABLE_HANDLER_TYPE_REQUIREMENTS)

    target_link_libraries(
        ${target_name}
        PRIVATE ${PROJECT_NAME}_shared Boost::unit_test_framework
                ${ARGS_PRIVATE}
        PUBLIC ${ARGS_PUBLIC})

    target_include_directories(
        ${target_name} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR})
    target_precompile_headers(${target_name} REUSE_FROM uh_precompile_headers)
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

uh_add_test(test_etcd PRIVATE etcd-cpp-api)
uh_add_test(test_etcd_manager PRIVATE etcd-cpp-api)
uh_add_test(test_etcd_manager_on_client_failure PRIVATE etcd-cpp-api)
uh_add_test(test_deduplicator PRIVATE uh_fake_global_data)
uh_add_test(test_address)
uh_add_test(test_awaitable_promise)
uh_add_test(test_big_int)
uh_add_test(test_data_store)
uh_add_test(test_fake_data_store PRIVATE uh_fake_storage)
uh_add_test(test_ec)
uh_add_test(test_fragment_set)
uh_add_test(test_global_data_view)
uh_add_test(test_hash)
uh_add_test(test_http_range)
uh_add_test(test_lfu_cache)
uh_add_test(test_license)
uh_add_test(test_lru_cache)
uh_add_test(test_plain_cache)
uh_add_test(test_pointer_traits)
uh_add_test(test_formats)
uh_add_test(test_pool)
uh_add_test(test_promise_future)
uh_add_test(test_reference_counter)
uh_add_test(test_scoped_buffer)
uh_add_test(test_serialization)
uh_add_test(test_service_registry)
uh_add_test(test_services)
uh_add_test(test_strings)
uh_add_test(test_xml_parser)

uh_add_test(test_variables PRIVATE uh_mockup_entrypoint)
uh_add_test(test_policies PRIVATE uh_mockup_entrypoint)
uh_add_test(test_policies_with_action PRIVATE uh_mockup_entrypoint)
uh_add_test(test_policies_with_condition PRIVATE uh_mockup_entrypoint)
uh_add_test(test_policies_with_examples PRIVATE uh_mockup_entrypoint)
