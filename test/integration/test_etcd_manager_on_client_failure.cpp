// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#define BOOST_TEST_MODULE "etcd manager tests"

#include <common/etcd/utils.h>

#include <boost/test/unit_test.hpp>

#include <memory>

using namespace std::chrono_literals;

namespace uh::cluster {

BOOST_AUTO_TEST_SUITE(when_client_has_system_failure_a_etcd_manager)

BOOST_AUTO_TEST_CASE(cannot_read_value_after_lease_time) {
    auto etcd = std::make_unique<etcd_manager>(etcd_config{}, 2 /*second*/);
    const auto value = std::string("172.0.0.1");
    etcd->put("/test/a", value);
    etcd.reset();

    std::this_thread::sleep_for(2.5s);
    etcd.reset(new etcd_manager({}, 2));
    auto read = etcd->get("/test/a");

    BOOST_TEST(read == "");
    etcd->clear_all();
}

BOOST_AUTO_TEST_CASE(
    cannot_read_value_always_because_lease_was_revoked_on_destroyer) {
    auto etcd = std::make_unique<etcd_manager>(etcd_config{}, 10 /*seconds*/);
    const auto value = std::string("172.0.0.1");
    etcd->put("/test/a", value);
    std::this_thread::sleep_for(1s);
    etcd.reset();
    std::this_thread::sleep_for(1s);

    etcd.reset(new etcd_manager({}, 10));
    std::this_thread::sleep_for(1s);
    auto read = etcd->get("/test/a");

    BOOST_TEST(read == "");
    etcd->clear_all();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
