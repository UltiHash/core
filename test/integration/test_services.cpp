#define BOOST_TEST_MODULE "service_registry tests"

#include <boost/test/unit_test.hpp>

#include <common/etcd/registry/service_id.h>
#include <common/etcd/registry/service_registry.h>
#include <common/etcd/service_discovery/service_load_balancer.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/etcd/service_discovery/storage_index.h>
#include <common/utils/common.h>
#include <storage/interfaces/data_store.h>

#include <util/checks.h>
#include <util/server.h>
#include <util/temp_directory.h>

using namespace boost::asio;

namespace uh::cluster {

struct fixture {
    temp_directory tmp;
    boost::asio::io_context ioc;
    etcd_manager etcd;
    std::size_t service_id;
    std::size_t num_storages = 5;
    storage_index services{num_storages};
    service_load_balancer<storage_interface> load_balancer;
    uh::cluster::service_maintainer<storage_interface> service_maintainer;

    fixture()
        : service_id{[this]() -> std::size_t {
              time_settings::instance().set_service_get_timeout(1s);

              return get_service_id(
                  etcd, get_service_string(storage_interface::service_role),
                  tmp.path());
          }()},
          service_maintainer(
              etcd, ns::root.storage_groups[0].storage_hostports,
              service_factory<storage_interface>(ioc, 2 /*num_connections*/),
              {services, load_balancer}) {}

    auto count_valid_services() {
        return std::ranges::count_if(services.get(),
                                     [](auto& s) { return s != nullptr; });
    }
    bool all_null_services() {
        return std::ranges::all_of(services.get(),
                                   [](auto& s) { return s == nullptr; });
    }
    auto get_storage_id(uint128_t pointer) {
        auto [id, _] = pointer_traits::rr::get_storage_pointer(pointer);
        return id;
    }
};

BOOST_FIXTURE_TEST_CASE(Empty, fixture) {
    BOOST_TEST(all_null_services());
    BOOST_CHECK_THROW(load_balancer.get(), std::exception);
    BOOST_CHECK_THROW(services.at(static_cast<std::size_t>(0u)),
                      std::exception);
}

BOOST_FIXTURE_TEST_CASE(DetectStateChange, fixture) {
    BOOST_TEST(all_null_services());

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(
            etcd, ns::root.storage_groups[0].storage_hostports[0], 8081);

        WAIT_UNTIL_CHECK(1000, count_valid_services() == 1u);
    }

    WAIT_UNTIL_CHECK(1000, all_null_services());
}

BOOST_FIXTURE_TEST_CASE(GetClient, fixture) {
    BOOST_TEST(all_null_services());

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(
            etcd, ns::root.storage_groups[0].storage_hostports[0], 8081);

        WAIT_UNTIL_NO_THROW(1000, load_balancer.get());
    }
}

BOOST_FIXTURE_TEST_CASE(Wait, fixture) {
    BOOST_TEST(all_null_services());

    {
        std::atomic<bool> has_result = false;
        std::thread waiter([&] {
            load_balancer.get();
            has_result = true;
        });

        CHECK_STABLE(100, !has_result);

        test::server srv("0.0.0.0", 8081);
        service_registry sr(
            etcd, ns::root.storage_groups[0].storage_hostports[0], 8081);

        WAIT_UNTIL_CHECK(100, has_result);

        waiter.join();
    }
}

BOOST_AUTO_TEST_CASE(FindInitial) {
    {
        fixture f;
        BOOST_TEST(f.all_null_services());
    }

    {
        test::server srv("0.0.0.0", 8081);
        etcd_manager etcd;
        service_registry sr(
            etcd, ns::root.storage_groups[0].storage_hostports[0], 8081);

        fixture f;
        BOOST_TEST(!f.all_null_services());
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

    BOOST_TEST(all_null_services());
    BOOST_CHECK_THROW(services.at(get_storage_id(uint128_t())), std::exception);

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(
            etcd, ns::root.storage_groups[0].storage_hostports[0], 8081);

        WAIT_UNTIL_CHECK(3000, count_valid_services() == 1u);
        BOOST_CHECK_NO_THROW(services.at(get_storage_id(uint128_t())));
    }

    WAIT_UNTIL_CHECK(3000, all_null_services());

    auto node_addr_range = pointer_traits::rr::get_global_pointer(
        data_store_config().max_data_store_size, 0, 1);
    (void)node_addr_range;

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr(
            etcd, ns::root.storage_groups[0].storage_hostports[1], 8081);

        WAIT_UNTIL_CHECK(3000, count_valid_services() == 1u);

        BOOST_CHECK_THROW(services.at(get_storage_id(uint128_t())),
                          std::exception);
        BOOST_CHECK_NO_THROW(
            services.at(get_storage_id(uint128_t(node_addr_range))));
        BOOST_CHECK_THROW(
            services.at(get_storage_id(uint128_t(node_addr_range * 2))),
            std::exception);
    }

    {
        test::server srv("0.0.0.0", 8081);
        service_registry sr1(
            etcd, ns::root.storage_groups[0].storage_hostports[1], 8081);
        service_registry sr2(
            etcd, ns::root.storage_groups[0].storage_hostports[3], 8081);

        WAIT_UNTIL_CHECK(3000, count_valid_services() == 2u);
        BOOST_CHECK_THROW(services.at(get_storage_id(uint128_t())),
                          std::exception);
        BOOST_CHECK_NO_THROW(
            services.at(get_storage_id(uint128_t(node_addr_range))));
        BOOST_CHECK_THROW(
            services.at(get_storage_id(uint128_t(node_addr_range * 2))),
            std::exception);
        BOOST_CHECK_NO_THROW(
            services.at(get_storage_id(uint128_t(node_addr_range * 3))));
    }
}

BOOST_FIXTURE_TEST_CASE(WaitForDependency, fixture) {
    BOOST_TEST(all_null_services());
    BOOST_CHECK_THROW(services.at(get_storage_id(uint128_t())),
                      std::runtime_error);

    {
        test::server svr("0.0.0.0", 8081);
        service_registry sr(
            etcd, ns::root.storage_groups[0].storage_hostports[0], 8081);

        WAIT_UNTIL_NO_THROW(1000, services.at(get_storage_id(uint128_t())));
    }
}

} // namespace uh::cluster
