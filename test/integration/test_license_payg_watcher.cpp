#define BOOST_TEST_MODULE "payg watcher tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/license/payg_updater.h>
#include <common/license/payg_watcher.h>
#include <fakeit/fakeit.hpp>
#include <lib/util/coroutine.h>

using namespace fakeit;

using namespace uh::cluster;
using namespace uh::cluster::lic;
using namespace boost::asio;

class fixture : public coro_fixture {
public:
    fixture()
        : coro_fixture{1},
          ioc{coro_fixture::get_io_context()},
          etcd{} {}

    io_context& ioc;
    etcd_manager etcd;

    // std::mutex mtx;
    // std::condition_variable cv;
    // bool io_context_started;

    static constexpr const char* json_literal = R"({
        "customer_id": "big corp xy",
        "license_type": "freemium",
        "storage_cap": 10240,
        "ec": {
            "enabled": true,
            "max_group_size": 10
        },
        "replication": {
            "enabled": true,
            "max_replicas": 3
        },
        "signature": 
        "yg2DNf6iej5np/rQuM4mkp1xzByxxV6vHmHjrbimLyNndL+biWhajraNcp88mXB6iNy/EQ5Izx8H6Q7mggpxBg=="
    })";
    static constexpr const char* json_compact_literal =
        R"({"customer_id":"big corp xy","ec":{"enabled":true,"max_group_size":10},"license_type":"freemium","replication":{"enabled":true,"max_replicas":3},"storage_cap":10240})";
};

BOOST_FIXTURE_TEST_SUITE(a_payg_updater, fixture)

BOOST_AUTO_TEST_CASE(updates_license_through_etcd) {
    auto sut = payg_updater(
        ioc, etcd, [&]() -> coro<std::string> { co_return json_literal; });

    auto future = co_spawn(ioc, sut.update(), use_future);
    future.get();

    BOOST_TEST(etcd.get(etcd_payg_license) == json_compact_literal);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_payg_watcher, fixture)

BOOST_AUTO_TEST_CASE(returns_updated_license_through_callback) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    payg received_license;
    payg_watcher sut{etcd, [&](const payg& license) {
                         received_license = license;
                         promise.set_value();
                     }};

    auto updater = payg_updater(
        ioc, etcd, [&]() -> coro<std::string> { co_return json_literal; });
    co_spawn(ioc, updater.update(), use_future).get();

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_CHECK_EQUAL(received_license.customer_id, "big corp xy");
    BOOST_CHECK_EQUAL(received_license.license_type, payg::type::FREEMIUM);
    BOOST_CHECK_EQUAL(received_license.storage_cap, 10240);
    BOOST_CHECK_EQUAL(received_license.ec.enabled, true);
    BOOST_CHECK_EQUAL(received_license.ec.max_group_size, 10);
    BOOST_CHECK_EQUAL(received_license.replication.enabled, true);
    BOOST_CHECK_EQUAL(received_license.replication.max_replicas, 3);
}

BOOST_AUTO_TEST_CASE(returns_updated_license_through_getter) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    payg_watcher sut{etcd, [&](const payg& license) { promise.set_value(); }};

    auto updater = payg_updater(
        ioc, etcd, [&]() -> coro<std::string> { co_return json_literal; });
    co_spawn(ioc, updater.update(), use_future).get();

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto received_license = sut.get();
    BOOST_CHECK_EQUAL(received_license.customer_id, "big corp xy");
    BOOST_CHECK_EQUAL(received_license.license_type, payg::type::FREEMIUM);
    BOOST_CHECK_EQUAL(received_license.storage_cap, 10240);
    BOOST_CHECK_EQUAL(received_license.ec.enabled, true);
    BOOST_CHECK_EQUAL(received_license.ec.max_group_size, 10);
    BOOST_CHECK_EQUAL(received_license.replication.enabled, true);
    BOOST_CHECK_EQUAL(received_license.replication.max_replicas, 3);
}

BOOST_AUTO_TEST_SUITE_END()
