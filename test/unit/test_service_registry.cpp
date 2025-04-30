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
        service_registry registering_registry(etcd, key);
        registering_registry.register_service({.port = port_address});

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
