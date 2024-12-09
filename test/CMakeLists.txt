include_directories(${CMAKE_SOURCE_DIR}/third-party)
configure_file(test_config.h.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.h)

add_library(uh_mockup_entrypoint SHARED gdv_fixture.h entrypoint.cpp)
target_link_libraries(uh_mockup_entrypoint PUBLIC ${PROJECT_NAME}_shared)
target_precompile_headers(uh_mockup_entrypoint REUSE_FROM uh_precompile_headers)

function(uh_add_test test_name)
    add_executable(${PROJECT_NAME}_${test_name} ${test_name}.cpp)
    target_link_libraries(${PROJECT_NAME}_${test_name}
                          PRIVATE ${PROJECT_NAME}_shared)
    target_include_directories(${PROJECT_NAME}_${test_name}
                               PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
    target_precompile_headers(${PROJECT_NAME}_${test_name} REUSE_FROM
                              uh_precompile_headers)
    add_test(${test_name} ${PROJECT_NAME}_${test_name}
             COMMAND ${PROJECT_NAME}_${test_name})
endfunction()

uh_add_test(test_address)
uh_add_test(test_awaitable_promise)
uh_add_test(test_big_int)
uh_add_test(test_data_store)
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

uh_add_test(test_variables)
target_link_libraries(${PROJECT_NAME}_test_variables
                      PRIVATE uh_mockup_entrypoint)
uh_add_test(test_policies)
target_link_libraries(${PROJECT_NAME}_test_policies
                      PRIVATE uh_mockup_entrypoint)
uh_add_test(test_policies_with_action)
target_link_libraries(${PROJECT_NAME}_test_policies_with_action
                      PRIVATE uh_mockup_entrypoint)
uh_add_test(test_policies_with_condition)
target_link_libraries(${PROJECT_NAME}_test_policies_with_condition
                      PRIVATE uh_mockup_entrypoint)
uh_add_test(test_policies_with_examples)
target_link_libraries(${PROJECT_NAME}_test_policies_with_examples
                      PRIVATE uh_mockup_entrypoint)
