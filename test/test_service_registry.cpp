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
    service_registry registering_registry(STORAGE_SERVICE, index, etcd);

    {
        // check if the keys already exist or not
        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));
        const auto announced_path_registry =
            etcd.get(get_announced_path(STORAGE_SERVICE, index));

        BOOST_CHECK(host.empty());
        BOOST_CHECK(port.empty());
        BOOST_CHECK(announced_path_registry.empty());
    }

    {
        // check for registry
        registering_registry.register_service({.port = port_address});

        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));
        const auto announced_etcd_path = std::filesystem::path(
            etcd.get(get_announced_path(STORAGE_SERVICE, index)));

        BOOST_CHECK(get_announced_id(announced_etcd_path) == index);
        BOOST_CHECK(announced_etcd_path ==
                    get_announced_path(STORAGE_SERVICE, index));
        BOOST_CHECK(host == boost::asio::ip::host_name());
        BOOST_CHECK(std::stoul(port) == port_address);
    }

    {
        // check for de-registry
        const auto host = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_HOST));
        const auto port = etcd.get(
            get_attributes_path(STORAGE_SERVICE, index) +
            get_etcd_service_attribute_string(uh::cluster::ENDPOINT_PORT));
        const auto announced_path_registry =
            etcd.get(get_announced_path(STORAGE_SERVICE, index));

        BOOST_CHECK(host.empty());
        BOOST_CHECK(port.empty());
        BOOST_CHECK(announced_path_registry.empty());
    }
}

} // end namespace uh::cluster
