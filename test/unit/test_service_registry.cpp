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

#include <boost/test/tools/old/interface.hpp>
#define BOOST_TEST_MODULE "service_registry tests"

#include "common/etcd/namespace.h"
#include "common/etcd/registry/service_registry.h"
#include "common/etcd/utils.h"
#include <boost/asio/ip/host_name.hpp>
#include <boost/test/unit_test.hpp>
#include <common/service_interfaces/hostport.h>
#include <common/utils/strings.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(basic_register_retrieve_deregister) {

    const auto index = 42;
    const auto port_address = 9200;

    auto etcd = etcd_manager();
    auto key = ns::root.deduplicator.hostports[index];

    {
        // check if the keys already exist or not
        auto hp = deserialize<hostport>(etcd.get(key));

        BOOST_TEST(hp.hostname.empty());
        BOOST_TEST(hp.port == 0);
    }

    {
        service_registry registering_registry(etcd, key, port_address);

        auto hp = deserialize<hostport>(etcd.get(key));
        BOOST_TEST(hp.hostname == boost::asio::ip::host_name());
        BOOST_TEST(hp.port == port_address);
    }

    {
        // check for de-registry
        auto hp = deserialize<hostport>(etcd.get(key));

        BOOST_TEST(hp.hostname.empty());
        BOOST_TEST(hp.port == 0);
    }
}

} // end namespace uh::cluster
