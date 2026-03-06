# Copyright 2026 UltiHash Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

set_property(GLOBAL PROPERTY TEST_TYPE "unit")

uh_add_test(test_address)
uh_add_test(test_http_client PRIVATE http_server network nlohmann_json)
uh_add_test(test_async_http_client PRIVATE http_server network nlohmann_json)
uh_add_test(test_backend_client PRIVATE http_server network nlohmann_json)
uh_add_test(test_task)
uh_add_test(test_cors PRIVATE entrypoint)
uh_add_test(test_cpr PRIVATE http_server nlohmann_json cpr::cpr)
uh_add_test(test_data_file PRIVATE storage)
uh_add_test(test_deduplicator PRIVATE uh_mock_global_data deduplicator)
uh_add_test(test_deletion_queue)
uh_add_test(test_disk_cache_body PRIVATE uh_mock_global_data entrypoint storage)
uh_add_test(test_disk_cache_manager PRIVATE uh_mock_global_data entrypoint storage)
uh_add_test(test_exp_backoff PRIVATE license)
uh_add_test(test_formats PRIVATE entrypoint)
uh_add_test(test_hash PRIVATE uhcrypto)
uh_add_test(test_http_range PRIVATE entrypoint)
uh_add_test(test_integral)
uh_add_test(test_lfu_cache)
uh_add_test(test_license PRIVATE license)
uh_add_test(test_namespace)
uh_add_test(test_opentelemetry PRIVATE telemetry)
uh_add_test(test_parallel_coroutines PRIVATE telemetry)
uh_add_test(test_plain_cache)
uh_add_test(test_pointer_traits)
uh_add_test(test_policies PRIVATE uh_mock_entrypoint)
uh_add_test(test_policies_with_action PRIVATE uh_mock_entrypoint)
uh_add_test(test_policies_with_condition PRIVATE uh_mock_entrypoint)
uh_add_test(test_policies_with_examples PRIVATE uh_mock_entrypoint)
uh_add_test(test_policy_parser PRIVATE entrypoint)
uh_add_test(test_pool)
uh_add_test(test_promise_future)
uh_add_test(test_proxy_cache)
uh_add_test(test_scoped_buffer)
uh_add_test(test_service_registry PRIVATE etcd)
add_executable(signals signals.cpp)
uh_add_test(test_signals)
add_dependencies(test_signals signals)
uh_add_test(test_storage_group_config PRIVATE group_data)
uh_add_test(test_storage_group_externals PRIVATE group_data storage_interfaces)
uh_add_test(test_storage_group_internals PRIVATE group_data)
uh_add_test(test_strings)
uh_add_test(test_trace_asio PRIVATE telemetry)
uh_add_test(test_variables PRIVATE uh_mock_entrypoint)
uh_add_test(test_xml_parser)
uh_add_test(test_beast)
