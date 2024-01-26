#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "services tests"
#endif


#include <boost/test/unit_test.hpp>
#include <boost/bind/bind.hpp>
#include "common/registry/service_registry.h"
#include "common/utils/services.h"

#define REGISTRY_ENDPOINT "http://127.0.0.1:2379"
#define WORKING_DIRECTORY "/var/lib/uh"


// ------------- Tests Suites Follow --------------

namespace uh::cluster {

    BOOST_AUTO_TEST_CASE ( basic_services_functionality_test ) {

        const auto index = 42;
        const auto port_address = 9200;

        auto ioc = boost::asio::io_context();

        config_registry config_registry(DEDUPLICATOR_SERVICE, REGISTRY_ENDPOINT, WORKING_DIRECTORY);

        services<STORAGE_SERVICE> storage_services(ioc, config_registry, 1, REGISTRY_ENDPOINT );

        {
            // before registration
            auto clients = storage_services.get_clients();
            BOOST_CHECK(clients.empty());
        }

        {
            service_registry registering_registry(STORAGE_SERVICE, index, REGISTRY_ENDPOINT);
            auto res = registering_registry.register_service({.port = port_address});

            BOOST_CHECK_THROW(storage_services.wait_for_dependency(), std::runtime_error);

            auto clients = storage_services.get_clients();
            BOOST_CHECK(clients.size() == 1);
        }

        {
            // after unregistering the service
            storage_services.cancel();
            auto clients = storage_services.get_clients();
            BOOST_CHECK(clients.empty());
        }
    }

} // end namespace uh::cluster
