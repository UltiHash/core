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
#include <common/utils/service_registry.h>

#define REGISTRY_ENDPOINT "http://127.0.0.1:2379"

// ------------- Tests Suites Follow --------------

namespace uh::cluster {

    BOOST_AUTO_TEST_CASE ( basic_register_retrieve_deregister )
    {
        service_registry querying_registry(get_service_string(uh::cluster::TEST_SERVICE) + "/0", REGISTRY_ENDPOINT);
        etcd::Client etcd_client(REGISTRY_ENDPOINT);
        std::string invalid_key = "/uh/" + get_service_string(uh::cluster::TEST_SERVICE) + "/42invalid";
        std::string global_key = "/uh/" + get_service_string(uh::cluster::TEST_SERVICE) + "/global";
        etcd_client.put(invalid_key, "invalid entry from test case");
        etcd_client.put(global_key, "global value");

        {
            auto service_endpoints = querying_registry.get_service_instances(uh::cluster::TEST_SERVICE);
            BOOST_CHECK(service_endpoints.empty());
        }

        {
            service_registry registering_registry(get_service_string(uh::cluster::TEST_SERVICE) + "/42", REGISTRY_ENDPOINT);
            registering_registry.register_service(23);

            auto service_endpoints = querying_registry.get_service_instances(uh::cluster::TEST_SERVICE);
            BOOST_CHECK(service_endpoints.size() == 1);
            BOOST_CHECK(service_endpoints.begin()->role == uh::cluster::TEST_SERVICE);
            BOOST_CHECK(service_endpoints.begin()->id == 42);
            BOOST_CHECK(service_endpoints.begin()->host == boost::asio::ip::host_name());
            BOOST_CHECK(service_endpoints.begin()->port == 23);
        }

        {
            auto service_endpoints = querying_registry.get_service_instances(uh::cluster::TEST_SERVICE);
            BOOST_CHECK(service_endpoints.empty());
        }

        etcd_client.rm(invalid_key);
        etcd_client.rm(global_key);
    }

    BOOST_AUTO_TEST_CASE( wait_for_dependencies, *boost::unit_test::timeout(15) )
    {
        service_registry querying_registry(get_service_string(uh::cluster::TEST_SERVICE) + "/0", REGISTRY_ENDPOINT);

        {
            auto service_endpoints = querying_registry.get_service_instances(uh::cluster::TEST_SERVICE);
            BOOST_CHECK(service_endpoints.empty());
        }

        service_registry registering_registry(get_service_string(uh::cluster::TEST_SERVICE) + "/42", REGISTRY_ENDPOINT);
        registering_registry.register_service(23);

        querying_registry.wait_for_dependency(uh::cluster::TEST_SERVICE);

        {
            auto service_endpoints = querying_registry.get_service_instances(uh::cluster::TEST_SERVICE);
            BOOST_CHECK(service_endpoints.size() == 1);
        }
    }
} // end namespace uh::cluster
