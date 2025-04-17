#include <boost/test/tools/old/interface.hpp>
#define BOOST_TEST_MODULE "service_registry tests"

#include "common/etcd/namespace.h"
#include "common/etcd/registry/service_registry.h"
#include "common/etcd/utils.h"
#include <boost/asio/ip/host_name.hpp>
#include <boost/test/unit_test.hpp>

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(basic_register_retrieve_deregister) {

    const auto index = 42;
    const auto port_address = 9200;

    auto etcd = etcd_manager();

    {
        // check if the keys already exist or not
        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));

        BOOST_TEST(host.empty());
        BOOST_TEST(port.empty());

        BOOST_TEST(!etcd.has(get_announced_path(STORAGE_SERVICE, index)));
    }

    {
        service_registry registering_registry(STORAGE_SERVICE, index, etcd);
        registering_registry.register_service({.port = port_address});

        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        BOOST_TEST(host == boost::asio::ip::host_name());

        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));
        BOOST_TEST(std::stoul(port) == port_address);

        const auto announced_etcd_path =
            get_announced_path(STORAGE_SERVICE, index);
        BOOST_TEST(get_announced_id(announced_etcd_path) == index);
        BOOST_TEST(etcd.has(announced_etcd_path));
    }

    {
        // check for de-registry
        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));

        BOOST_TEST(host.empty());
        BOOST_TEST(port.empty());

        BOOST_TEST(!etcd.has(get_announced_path(STORAGE_SERVICE, index)));
    }
}

} // end namespace uh::cluster
