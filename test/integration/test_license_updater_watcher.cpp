#define BOOST_TEST_MODULE "license updater/watcher tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/license/updater.h>
#include <common/license/watcher.h>
#include <fakeit/fakeit.hpp>
#include <lib/util/coroutine.h>

using namespace fakeit;

using namespace uh::cluster;
using namespace boost::asio;

class fixture : public coro_fixture {
public:
    fixture()
        : coro_fixture{1},
          ioc{coro_fixture::get_io_context()},
          etcd{} {}

    io_context& ioc;
    etcd_manager etcd;

    static constexpr const char* json_literal = R"({
        "version": "v1",
        "customer_id": "big corp xy",
        "license_type": "freemium",
        "storage_cap": 10240,
        "signature": 
        "YcLv4CtuxTpZ1N4bnRft0B8xKF1ecAaHCUJK9F4dy8VuL3wcRo9Mu2+LyVwSSeu2C4xgWnKO3WkAWUszAXy8Dw=="
    })";

    static constexpr const char* json_compact_literal =
        R"({"version":"v1","customer_id":"big corp xy","license_type":"freemium","storage_cap":10240,"signature":"YcLv4CtuxTpZ1N4bnRft0B8xKF1ecAaHCUJK9F4dy8VuL3wcRo9Mu2+LyVwSSeu2C4xgWnKO3WkAWUszAXy8Dw=="})";
};

BOOST_FIXTURE_TEST_SUITE(a_license_updater, fixture)

BOOST_AUTO_TEST_CASE(updates_license_through_etcd) {
    auto sut = license_updater(ioc, etcd, pseudo_backend_client(json_literal));

    auto future = co_spawn(ioc, sut.update(), use_future);
    future.get();

    BOOST_TEST(etcd.get(etcd_license) == json_compact_literal);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_license_watcher, fixture)

BOOST_AUTO_TEST_CASE(returns_updated_license_through_getter) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    auto sut =
        license_watcher{etcd, [&](std::string_view) { promise.set_value(); }};

    auto updater =
        license_updater(ioc, etcd, pseudo_backend_client(json_literal));
    co_spawn(ioc, updater.update(), use_future).get();

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto received_lic = sut.get_license();
    BOOST_CHECK_EQUAL(received_lic.customer_id, "big corp xy");
    BOOST_CHECK_EQUAL(received_lic.license_type, license::type::FREEMIUM);
    BOOST_CHECK_EQUAL(received_lic.storage_cap, 10240);
}

BOOST_AUTO_TEST_SUITE_END()
