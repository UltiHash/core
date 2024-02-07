#ifdef SINGLE_TEST_RUNNER
#define BOOST_TEST_NO_MAIN
#else
#define BOOST_TEST_MODULE "service_registry tests"
#endif

#include <boost/test/unit_test.hpp>

#include "common/utils/common.h"
#include "common/utils/temp_dir.h"
#include "common/registry/service_registry.h"
#include "common/registry/services.h"
#include "common/test/checks.h"
#include "common/test/server.h"


#define REGISTRY_ENDPOINT "http://127.0.0.1:2379"

using namespace boost::asio;

namespace uh::cluster {

template <role r, role service_role = r>
struct base_fixture
{
    temp_directory tmp;
    boost::asio::io_context ioc;
    config_registry reg;
    uh::cluster::services<r> services;

    base_fixture()
        : reg(service_role, REGISTRY_ENDPOINT, tmp.path()),
          services(ioc, reg, 2, REGISTRY_ENDPOINT)
    {
    }
};

using fixture = base_fixture<DEDUPLICATOR_SERVICE>;

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

BOOST_FIXTURE_TEST_CASE(Wait, fixture)
{
    BOOST_CHECK(services.get_clients().empty());

    {
        std::atomic<bool> has_result = false;
        std::thread waiter([&]{
            services.get();
            has_result = true;
        });

        CHECK_STABLE(100, !has_result);

        test::server srv("0.0.0.0", 8081);
        service_registry sr(DEDUPLICATOR_SERVICE, 0, REGISTRY_ENDPOINT);
        auto reg = sr.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});

        WAIT_UNTIL_CHECK(100, has_result);

        waiter.join();
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

BOOST_FIXTURE_TEST_CASE(GetClientById, fixture)
{
    BOOST_CHECK(services.get_clients().empty());

    std::size_t test_id = 0xdeadbeef;

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(DEDUPLICATOR_SERVICE, test_id, REGISTRY_ENDPOINT);
        auto reg = sr.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});

        WAIT_UNTIL_CHECK(1000, services.get_clients().size() == 1u);
        BOOST_CHECK_THROW(services.get(std::size_t{}), std::exception);
        BOOST_CHECK_NO_THROW(services.get(test_id));
    }
}

using dedup_fixture = base_fixture<STORAGE_SERVICE, DEDUPLICATOR_SERVICE>;
BOOST_FIXTURE_TEST_CASE(GetClientByOffset, dedup_fixture)
{
    /* Note: we are checking implementation details here. The following
     * assumptions must hold true for this test to succeed. If they are not
     * true anymore, you should refactor/delete this test.
     *
     * - each storage service owns the same amount of space which is defined
     *   by max_data_store_size in global_data_view_config
     * - each nodes storage offset is determined by product of the node's id
     *   and max_data_store_size
     */
    auto node_addr_range = reg.get_global_data_view_config().max_data_store_size;

    BOOST_CHECK(services.get_clients().empty());
    BOOST_CHECK_THROW(services.get(uint128_t()), std::exception);

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 0, REGISTRY_ENDPOINT);
        auto reg = sr.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});

        WAIT_UNTIL_CHECK(1000, services.get_clients().size() == 1u);
        BOOST_CHECK_NO_THROW(services.get(uint128_t()));
    }

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 1, REGISTRY_ENDPOINT);
        auto reg = sr.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});

        WAIT_UNTIL_CHECK(1000, services.get_clients().size() == 1u);
        BOOST_CHECK_THROW(services.get(uint128_t()), std::exception);
        BOOST_CHECK_NO_THROW(services.get(uint128_t(node_addr_range)));
        BOOST_CHECK_THROW(services.get(uint128_t(node_addr_range * 2)), std::exception);
    }

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr1(STORAGE_SERVICE, 1, REGISTRY_ENDPOINT);
        auto reg1 = sr1.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});
        service_registry sr2(STORAGE_SERVICE, 3, REGISTRY_ENDPOINT);
        auto reg2 = sr2.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});

        WAIT_UNTIL_CHECK(1000, services.get_clients().size() == 2u);
        BOOST_CHECK_THROW(services.get(uint128_t()), std::exception);
        BOOST_CHECK_NO_THROW(services.get(uint128_t(node_addr_range)));
        BOOST_CHECK_THROW(services.get(uint128_t(node_addr_range * 2)), std::exception);
        BOOST_CHECK_NO_THROW(services.get(uint128_t(node_addr_range * 3)));
    }
}

BOOST_FIXTURE_TEST_CASE(WaitForDependency, dedup_fixture)
{
    BOOST_CHECK(services.get_clients().empty());
    BOOST_CHECK_THROW(services.get(uint128_t()), std::runtime_error);

    {
        test::server svr("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 0, REGISTRY_ENDPOINT);
        auto reg = sr.register_service({ .threads = 1, .port=8081, .bind_address="localhost"});

        WAIT_UNTIL_NO_THROW(1000, services.get(uint128_t()));
    }
}
}
