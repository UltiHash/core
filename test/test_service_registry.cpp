#define BOOST_TEST_MODULE "service_registry tests"

#include "common/etcd/namespace.h"
#include "common/etcd/registry/service_registry.h"
#include <boost/asio/ip/host_name.hpp>
#include <boost/test/unit_test.hpp>

#define REGISTRY_ENDPOINT "http://127.0.0.1:2379"

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

BOOST_AUTO_TEST_CASE(basic_register_retrieve_deregister) {

    const auto index = 42;
    const auto port_address = 9200;

    auto etcd_client = etcd::SyncClient(REGISTRY_ENDPOINT);
    service_registry registering_registry(STORAGE_SERVICE, index, etcd_client);

    {
        // check if the keys already exist or not
        const auto host = etcd_client
                              .get(get_attributes_path(STORAGE_SERVICE, index) +
                                   get_etcd_service_attribute_string(
                                       uh::cluster::ENDPOINT_HOST))
                              .value()
                              .as_string();
        const auto port = etcd_client
                              .get(get_attributes_path(STORAGE_SERVICE, index) +
                                   get_etcd_service_attribute_string(
                                       uh::cluster::ENDPOINT_PORT))
                              .value()
                              .as_string();
        const auto announced_path_registry =
            etcd_client.get(get_announced_path(STORAGE_SERVICE, index))
                .value()
                .key();

        BOOST_CHECK(host.empty());
        BOOST_CHECK(port.empty());
        BOOST_CHECK(announced_path_registry.empty());
    }

    {
        // check for registry
        auto reg =
            registering_registry.register_service({.port = port_address});

        const auto host = etcd_client
                              .get(get_attributes_path(STORAGE_SERVICE, index) +
                                   get_etcd_service_attribute_string(
                                       uh::cluster::ENDPOINT_HOST))
                              .value()
                              .as_string();
        const auto port = etcd_client
                              .get(get_attributes_path(STORAGE_SERVICE, index) +
                                   get_etcd_service_attribute_string(
                                       uh::cluster::ENDPOINT_PORT))
                              .value()
                              .as_string();
        const auto announced_etcd_path = std::filesystem::path(
            etcd_client.get(get_announced_path(STORAGE_SERVICE, index))
                .value()
                .key());

        BOOST_CHECK(get_announced_id(announced_etcd_path) == index);
        BOOST_CHECK(announced_etcd_path ==
                    get_announced_path(STORAGE_SERVICE, index));
        BOOST_CHECK(host == boost::asio::ip::host_name());
        BOOST_CHECK(std::stoul(port) == port_address);
    }

    {
        // check for de-registry
        const auto host = etcd_client
                              .get(get_attributes_path(STORAGE_SERVICE, index) +
                                   get_etcd_service_attribute_string(
                                       uh::cluster::ENDPOINT_HOST))
                              .value()
                              .as_string();
        const auto port = etcd_client
                              .get(get_attributes_path(STORAGE_SERVICE, index) +
                                   get_etcd_service_attribute_string(
                                       uh::cluster::ENDPOINT_PORT))
                              .value()
                              .as_string();
        const auto announced_path_registry =
            etcd_client.get(get_announced_path(STORAGE_SERVICE, index))
                .value()
                .key();

        BOOST_CHECK(host.empty());
        BOOST_CHECK(port.empty());
        BOOST_CHECK(announced_path_registry.empty());
    }
}

} // end namespace uh::cluster
