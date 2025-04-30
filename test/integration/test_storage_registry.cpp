#include <boost/test/tools/old/interface.hpp>
#define BOOST_TEST_MODULE "storage_registry tests"

#include "common/etcd/namespace.h"
#include "common/etcd/utils.h"
#include <boost/asio/ip/host_name.hpp>
#include <boost/test/unit_test.hpp>
#include <common/etcd/registry/storage_registry.h>
#include <common/service_interfaces/hostport.h>
#include <common/utils/strings.h>
#include <util/temp_directory.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster::storage {

BOOST_AUTO_TEST_CASE(basic_register_retrieve_deregister) {

    const auto service_id = 23;
    const auto group_id = 42;
    const auto port_address = 9200;

    auto etcd = etcd_manager();
    auto key = ns::root.storage_groups[group_id].storage_hostports[service_id];

    {
        // check if the keys already exist or not
        auto hp = deserialize<hostport>(etcd.get(key));

        BOOST_TEST(hp.hostname.empty());
        BOOST_TEST(hp.port == 0);
    }

    {
        temp_directory tmp_dir;
        storage_registry registering_registry(etcd, group_id, service_id,
                                              tmp_dir.path());
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

} // namespace uh::cluster::storage
