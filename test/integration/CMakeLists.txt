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

set_property(GLOBAL PROPERTY TEST_TYPE "integration")

uh_add_test(test_gdv_fixture PRIVATE storage coordinator)
uh_add_test(test_fragment_set PRIVATE deduplicator)
uh_add_test(test_global_data_view PRIVATE storage coordinator)

uh_add_test(test_gdv_fixture PRIVATE storage coordinator MODE with_ec)
uh_add_test(test_global_data_view PRIVATE storage coordinator MODE with_ec)
uh_add_test(test_fragment_set PRIVATE deduplicator MODE with_ec)

uh_add_test(test_awaitable_promise PRIVATE network)
uh_add_test(test_coordinator PRIVATE etcd storage coordinator)
uh_add_test(test_data_store PRIVATE storage)
uh_add_test(test_ec PRIVATE ec)
uh_add_test(test_ec_data_view PRIVATE storage)
uh_add_test(test_etcd_sync_client PRIVATE etcd-cpp-api)
uh_add_test(test_etcd_manager PRIVATE etcd)
uh_add_test(test_etcd_manager_on_client_failure PRIVATE etcd)
uh_add_test(test_license_updater_watcher PRIVATE etcd license)
uh_add_test(test_lru_cache)
uh_add_test(test_mock_data_store PRIVATE uh_mock_storage)
uh_add_test(test_reference_counter PRIVATE storage)
uh_add_test(test_service PRIVATE entrypoint deduplicator storage)
uh_add_test(test_service_discovery PRIVATE etcd storage)
uh_add_test(test_storage_state_manager PRIVATE etcd storage)
uh_add_test(test_ec_maintainer PRIVATE storage)
uh_add_test(test_subscriber PRIVATE etcd)

uh_add_test(test_storage_outage PRIVATE storage coordinator)
add_dependencies(test_storage_outage uh-cluster)

