#define BOOST_TEST_MODULE "etcd manager tests"

#include "common/etcd/utils.h"
#include "common/telemetry/log.h"
#include "fakeit.hpp"

#include <boost/test/unit_test.hpp>
#include <future>
#include <ranges>

using namespace fakeit;
using namespace std::chrono_literals;

namespace uh::cluster {

class callback_interface {
public:
    virtual ~callback_interface() = default;
    virtual void handle_state_changes(const etcd::Response& response) = 0;
};

class fixture {
public:
    void setup() {
        auto log_config = log::config{
            .sinks = {log::sink_config{.type = log::sink_type::cout,
                                       .level = boost::log::trivial::debug,
                                       .service_role = DEDUPLICATOR_SERVICE}}};
        log::init(log_config);
    }

    fixture()
        : etcd{} {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~fixture() {
        etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    etcd_config cfg;
    etcd_manager etcd;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(a_etcd_manager)

BOOST_FIXTURE_TEST_CASE(returns_the_written_value_through_get, fixture) {
    const auto value = std::string("172.0.0.1");
    etcd.put("/test/a", value);

    auto read = etcd.get("/test/a");
    BOOST_TEST(value == read);
}

BOOST_FIXTURE_TEST_CASE(returns_all_keys_under_the_given_path, fixture) {
    const auto value = std::string("172.0.0.1");
    auto keys = std::vector<std::string>{"/test/a", "/test/b"};
    etcd.put(keys[0], "0");
    etcd.put(keys[1], "1");

    auto read = etcd.keys("/test");

    BOOST_REQUIRE_EQUAL_COLLECTIONS(read.begin(), read.end(), keys.begin(),
                                    keys.end());
}

BOOST_FIXTURE_TEST_CASE(returns_all_key_value_pairs_under_the_given_path,
                        fixture) {
    const auto value = std::string("172.0.0.1");
    auto keys = std::vector<std::string>{"/test/a", "/test/b"};
    auto values = std::vector<std::string>{"0", "1"};
    etcd.put(keys[0], values[0]);
    etcd.put(keys[1], values[1]);

    auto read = etcd.ls("/test");
    std::vector<std::string> read_keys;
    std::vector<std::string> read_values;
    std::ranges::copy(read | std::views::keys, std::back_inserter(read_keys));
    std::ranges::copy(read | std::views::values,
                      std::back_inserter(read_values));

    BOOST_REQUIRE_EQUAL_COLLECTIONS(read_keys.begin(), read_keys.end(),
                                    keys.begin(), keys.end());
    BOOST_REQUIRE_EQUAL_COLLECTIONS(read_values.begin(), read_values.end(),
                                    values.begin(), values.end());
}

BOOST_FIXTURE_TEST_CASE(clears_all_keys_under_the_given_path, fixture) {
    const auto value = std::string("172.0.0.1");
    auto keys = std::vector<std::string>{"/test/a", "/test/b"};
    etcd.put(keys[0], "0");
    etcd.put(keys[1], "1");

    etcd.rmdir("/test");

    auto read = etcd.keys("/test/");
    BOOST_TEST(read.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(clears_all, fixture) {
    const auto value = std::string("172.0.0.1");
    auto keys = std::vector<std::string>{"/test/a", "/test/b"};
    etcd.put(keys[0], "0");
    etcd.put(keys[1], "1");

    etcd.clear_all();

    auto read = etcd.keys("/test/");
    BOOST_TEST(read.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(watches_write, fixture) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    auto wm = etcd.watch(
        "/test", [&](const etcd::Response& response) { promise.set_value(); });

    etcd.put("/test/sub/a0", "172.0.0.1");

    BOOST_CHECK(future.wait_for(2s) != std::future_status::timeout);
}

BOOST_FIXTURE_TEST_CASE(watch_rewrite, fixture) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    etcd.put("/test/sub/a0", "172.0.0.1");
    auto wm = etcd.watch(
        "/test", [&](const etcd::Response& response) { promise.set_value(); });

    etcd.put("/test/sub/a0", "172.0.0.1");

    BOOST_CHECK(future.wait_for(2s) != std::future_status::timeout);
}

BOOST_FIXTURE_TEST_CASE(watches_overwrite, fixture) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    etcd.put("/test/sub/a0", "198.51.100.0");
    auto wm = etcd.watch(
        "/test", [&](const etcd::Response& response) { promise.set_value(); });

    etcd.put("/test/sub/a0", "172.0.0.1");

    BOOST_CHECK(future.wait_for(2s) != std::future_status::timeout);
}

BOOST_FIXTURE_TEST_CASE(watches_remove, fixture) {
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    etcd.put("/test/sub/a0", "172.0.0.1");
    auto wm = etcd.watch(
        "/test", [&](const etcd::Response& response) { promise.set_value(); });

    etcd.rmdir("/test");

    BOOST_CHECK(future.wait_for(2s) != std::future_status::timeout);
}

BOOST_FIXTURE_TEST_CASE(
    returns_lock_guard_and_its_destroyer_doesnt_throw_any_exception, fixture) {
    BOOST_CHECK_NO_THROW(
        { auto lock_guard = etcd.get_lock_guard("/foo/bar"); });
}

BOOST_FIXTURE_TEST_CASE(
    can_get_lock_from_same_key_after_first_lock_is_destroyed, fixture) {
    { auto lock_guard = etcd.get_lock_guard("/foo/bar"); }

    BOOST_CHECK_NO_THROW(
        { auto lock_guard = etcd.get_lock_guard("/foo/bar"); });
}

BOOST_FIXTURE_TEST_CASE(_waits_on_second_lock_until_first_lock_is_unlocked,
                        fixture) {

    std::optional<std::future<void>> future;
    {
        auto lock_guard = etcd.get_lock_guard("/foo/bar");

        // Emulate second client
        future = std::async(std::launch::async, []() {
            etcd_manager etcd;
            auto lock_guard = etcd.get_lock_guard("/foo/bar");
        });

        std::this_thread::sleep_for(std::chrono::seconds(2));
        BOOST_CHECK(future->wait_for(std::chrono::seconds(0)) ==
                    std::future_status::timeout);
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
