//
// Created by max on 22.12.23.
//

#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "service_registry tests"
#endif

#include <boost/test/unit_test.hpp>
#include <boost/asio/ip/host_name.hpp>
#include "common/registry/service_registry.h"

#define REGISTRY_ENDPOINT "http://127.0.0.1:2379"

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

    BOOST_AUTO_TEST_CASE ( basic_register_retrieve_deregister )
    {

        const auto index = 42;
        const auto port_address = 9200;

        auto etcd_client = etcd::SyncClient(REGISTRY_ENDPOINT);
        service_registry registering_registry(STORAGE_SERVICE, index, REGISTRY_ENDPOINT);

        const auto service_prefix_path = etcd_services_attributes_key_prefix + get_service_string(STORAGE_SERVICE) + '/' + std::to_string(index) + '/';
        const auto announced_path = etcd_services_announced_key_prefix + get_service_string(STORAGE_SERVICE) + '/' + std::to_string(index);

        {
            // check if the keys already exist or not
            const auto host = etcd_client.get(service_prefix_path + get_config_string(uh::cluster::CFG_ENDPOINT_HOST)).value().as_string();
            const auto port = etcd_client.get(service_prefix_path + get_config_string(uh::cluster::CFG_ENDPOINT_PORT)).value().as_string();
            const auto announced_path_registry = etcd_client.get(announced_path).value().key();

            BOOST_CHECK(host.empty());
            BOOST_CHECK(port.empty());
            BOOST_CHECK(announced_path_registry.empty());
        }

        {
            // check for registry
            auto reg = registering_registry.register_service({.port = port_address});

            const auto host = etcd_client.get(service_prefix_path + get_config_string(uh::cluster::CFG_ENDPOINT_HOST)).value().as_string();
            const auto port = etcd_client.get(service_prefix_path + get_config_string(uh::cluster::CFG_ENDPOINT_PORT)).value().as_string();
            const auto announced_etcd_path = std::filesystem::path(etcd_client.get(announced_path).value().key());

            BOOST_CHECK(std::stoi(announced_etcd_path.filename()) == index);
            BOOST_CHECK(announced_etcd_path.parent_path().filename() == get_service_string(STORAGE_SERVICE));
            BOOST_CHECK(host == boost::asio::ip::host_name());
            BOOST_CHECK(std::stoul(port) == port_address);
        }

        {
            // check for de-registry
            const auto host = etcd_client.get(service_prefix_path + get_config_string(uh::cluster::CFG_ENDPOINT_HOST)).value().as_string();
            const auto port = etcd_client.get(service_prefix_path + get_config_string(uh::cluster::CFG_ENDPOINT_PORT)).value().as_string();
            const auto announced_path_registry = etcd_client.get(announced_path).value().key();

            BOOST_CHECK(host.empty());
            BOOST_CHECK(port.empty());
            BOOST_CHECK(announced_path_registry.empty());
        }

    }

} // end namespace uh::cluster
