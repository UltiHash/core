#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "service_registry tests"
#endif

#include <boost/test/unit_test.hpp>

#include "common/utils/common.h"
#include "common/utils/temp_dir.h"
#include "common/registry/service_registry.h"
#include "common/utils/services.h"
#include "common/test/checks.h"
#include "common/test/server.h"


#define REGISTRY_ENDPOINT "http://127.0.0.1:2379"

using namespace boost::asio;

namespace uh::cluster {

struct fixture
{
    temp_directory tmp;
    boost::asio::io_context ioc;
    config_registry reg;
    uh::cluster::services<DEDUPLICATOR_SERVICE> services;

    fixture()
        : reg(DEDUPLICATOR_SERVICE, REGISTRY_ENDPOINT, tmp.path()),
          services(ioc, reg, 2, REGISTRY_ENDPOINT)
    {
    }
};

BOOST_FIXTURE_TEST_CASE(Empty, fixture)
{
    BOOST_CHECK(services.get_clients().empty());
    BOOST_CHECK_THROW(services.get(), std::exception);
    BOOST_CHECK_THROW(services.get(static_cast<std::size_t>(0u)), std::exception);
}

BOOST_FIXTURE_TEST_CASE(DetectStateChange, fixture)
{
    BOOST_CHECK(services.get_clients().empty());

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(DEDUPLICATOR_SERVICE, 0, REGISTRY_ENDPOINT);
        auto reg = sr.register_service({ .threads = 1, .port=8081, .bind_address="localhost" });

        {
            WAIT_UNTIL_CHECK(1000, services.get_clients().size() == 1u);
        }
    }

    WAIT_UNTIL_CHECK(1000, services.get_clients().empty());
}

BOOST_FIXTURE_TEST_CASE(GetClient, fixture)
{
    BOOST_CHECK(services.get_clients().empty());

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(DEDUPLICATOR_SERVICE, 0, REGISTRY_ENDPOINT);
        auto reg = sr.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});

        {
            WAIT_UNTIL_NO_THROW(1000, services.get());
        }
    }
}

BOOST_AUTO_TEST_CASE(FindInitial)
{
    {
        fixture f;
        BOOST_CHECK(f.services.get_clients().empty());
    }

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(DEDUPLICATOR_SERVICE, 0, REGISTRY_ENDPOINT);
        auto reg = sr.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});

        fixture f;
        BOOST_CHECK(!f.services.get_clients().empty());
    }
}

}
