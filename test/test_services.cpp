#define BOOST_TEST_MODULE "service_registry tests"

#include <boost/test/unit_test.hpp>

#include "common/etcd/registry/service_id.h"
#include "common/etcd/registry/service_registry.h"
#include "common/etcd/service_discovery/roundrobin_load_balancer.h"
#include "common/etcd/service_discovery/service_maintainer.h"
#include "common/etcd/service_discovery/storage_service_get_handler.h"
#include "common/test/checks.h"
#include "common/test/server.h"
#include "common/utils/common.h"
#include "common/utils/temp_directory.h"
#include "storage/data_store.h"

#define REGISTRY_ENDPOINT "http://127.0.0.1:2379"

using namespace boost::asio;

namespace uh::cluster {

struct fixture {
    temp_directory tmp;
    boost::asio::io_context ioc;
    etcd::SyncClient etcd_client;
    std::size_t service_id;
    storage_service_get_handler services;
    roundrobin_load_balancer<storage_interface> load_balancer;
    uh::cluster::service_maintainer<storage_interface> service_maintainer;

    uh::cluster::service_maintainer<storage_interface> make_services() {
        return uh::cluster::service_maintainer<storage_interface>(
            etcd_client, service_factory<storage_interface>(ioc, 2, nullptr));
    }

    fixture()
        : etcd_client(REGISTRY_ENDPOINT),
          service_id(get_service_id(
              etcd_client, get_service_string(storage_interface::service_role),
              tmp.path())),
          service_maintainer(make_services()) {
        service_maintainer.add_monitor(services);
        service_maintainer.add_monitor(load_balancer);
    }
};

BOOST_FIXTURE_TEST_CASE(Empty, fixture) {
    BOOST_CHECK(services.get_services().empty());
    BOOST_CHECK_THROW(load_balancer.get(), std::exception);
    BOOST_CHECK_THROW(services.get(static_cast<std::size_t>(0u)),
                      std::exception);
}

BOOST_FIXTURE_TEST_CASE(DetectStateChange, fixture) {
    BOOST_CHECK(services.get_services().empty());

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 0, etcd_client);
        auto reg = sr.register_service({.port = 8081});

        { WAIT_UNTIL_CHECK(1000, services.get_services().size() == 1u); }
    }

    WAIT_UNTIL_CHECK(1000, services.get_services().empty());
}

BOOST_FIXTURE_TEST_CASE(GetClient, fixture) {
    BOOST_CHECK(services.get_services().empty());

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 0, etcd_client);
        auto reg = sr.register_service({.port = 8081});

        { WAIT_UNTIL_NO_THROW(1000, load_balancer.get()); }
    }
}

BOOST_FIXTURE_TEST_CASE(Wait, fixture) {
    BOOST_CHECK(services.get_services().empty());

    {
        std::atomic<bool> has_result = false;
        std::thread waiter([&] {
            load_balancer.get();
            has_result = true;
        });

        CHECK_STABLE(100, !has_result);

        test::server srv("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 0, etcd_client);
        auto reg = sr.register_service({.port = 8081});

        WAIT_UNTIL_CHECK(100, has_result);

        waiter.join();
    }
}

BOOST_AUTO_TEST_CASE(FindInitial) {
    {
        fixture f;
        BOOST_CHECK(f.services.get_services().empty());
    }

    {
        test::server srv("0.0.0.0", 8081);
        etcd::SyncClient etcd_client(REGISTRY_ENDPOINT);
        service_registry sr(STORAGE_SERVICE, 0, etcd_client);
        auto reg = sr.register_service({.port = 8081});

        fixture f;
        BOOST_CHECK(!f.services.get_services().empty());
    }
}

BOOST_FIXTURE_TEST_CASE(GetClientById, fixture) {
    BOOST_CHECK(services.get_services().empty());

    std::size_t test_id = 0xdeadbeef;

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, test_id, etcd_client);
        auto reg = sr.register_service({.port = 8081});

        WAIT_UNTIL_CHECK(1000, services.get_services().size() == 1u);
        BOOST_CHECK_THROW(services.get(std::size_t{}), std::exception);
        BOOST_CHECK_NO_THROW(services.get(test_id));
    }
}

BOOST_FIXTURE_TEST_CASE(GetClientByOffset, fixture) {
    /* Note: we are checking implementation details here. The following
     * assumptions must hold true for this test to succeed. If they are not
     * true anymore, you should refactor/delete this test.
     *
     * - each storage service owns the same amount of space which is defined
     *   by max_data_store_size in global_data_view_config
     * - each nodes storage offset is determined by product of the node's id
     *   and max_data_store_size
     */

    auto node_addr_range = pointer_traits::get_global_pointer(
        data_store_config().max_data_store_size, 1, 0);

    BOOST_CHECK(services.get_services().empty());
    BOOST_CHECK_THROW(services.get(uint128_t()), std::exception);

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 0, etcd_client);
        auto reg = sr.register_service({.port = 8081});

        WAIT_UNTIL_CHECK(3000, services.get_services().size() == 1u);
        BOOST_CHECK_NO_THROW(services.get(uint128_t()));
    }

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 1, etcd_client);
        auto reg = sr.register_service({.port = 8081});

        WAIT_UNTIL_CHECK(3000, services.get_services().size() == 1u);
        BOOST_CHECK_THROW(services.get(uint128_t()), std::exception);
        BOOST_CHECK_NO_THROW(services.get(uint128_t(node_addr_range)));
        BOOST_CHECK_THROW(services.get(uint128_t(node_addr_range * 2)),
                          std::exception);
    }

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr1(STORAGE_SERVICE, 1, etcd_client);
        auto reg1 = sr1.register_service({.port = 8081});
        service_registry sr2(STORAGE_SERVICE, 3, etcd_client);
        auto reg2 = sr2.register_service({.port = 8081});

        WAIT_UNTIL_CHECK(3000, services.get_services().size() == 2u);
        BOOST_CHECK_THROW(services.get(uint128_t()), std::exception);
        BOOST_CHECK_NO_THROW(services.get(uint128_t(node_addr_range)));
        BOOST_CHECK_THROW(services.get(uint128_t(node_addr_range * 2)),
                          std::exception);
        BOOST_CHECK_NO_THROW(services.get(uint128_t(node_addr_range * 3)));
    }
}

BOOST_FIXTURE_TEST_CASE(WaitForDependency, fixture) {
    BOOST_CHECK(services.get_services().empty());
    BOOST_CHECK_THROW(services.get(uint128_t()), std::runtime_error);

    {
        test::server svr("0.0.0.0", 8081);
        service_registry sr(STORAGE_SERVICE, 0, etcd_client);
        auto reg = sr.register_service({.port = 8081});

        WAIT_UNTIL_NO_THROW(1000, services.get(uint128_t()));
    }
}

} // namespace uh::cluster
