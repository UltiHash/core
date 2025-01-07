#define BOOST_TEST_MODULE "etcd manager tests"

#include "common/etcd/utils.h"
#include "common/telemetry/log.h"
#include "fakeit/fakeit.hpp"
#include "utils/system.h"

#include <boost/test/unit_test.hpp>
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
        : manager{} {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~fixture() {
        manager.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    etcd_config cfg;
    etcd_manager manager;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(a_etcd_manager)

BOOST_FIXTURE_TEST_CASE(returns_the_written_value_through_get, fixture) {
    const auto value = std::string("172.0.0.1");
    manager.put("/test/a", value);

    auto read = manager.get("/test/a");
    BOOST_TEST(value == read);
}

BOOST_FIXTURE_TEST_CASE(returns_all_keys_under_the_given_path, fixture) {
    const auto value = std::string("172.0.0.1");
    auto keys = std::vector<std::string>{"/test/a", "/test/b"};
    manager.put(keys[0], "0");
    manager.put(keys[1], "1");

    auto read = manager.keys("/test");

    BOOST_REQUIRE_EQUAL_COLLECTIONS(read.begin(), read.end(), keys.begin(),
                                    keys.end());
}

BOOST_FIXTURE_TEST_CASE(returns_all_key_value_pairs_under_the_given_path,
                        fixture) {
    const auto value = std::string("172.0.0.1");
    auto keys = std::vector<std::string>{"/test/a", "/test/b"};
    auto values = std::vector<std::string>{"0", "1"};
    manager.put(keys[0], values[0]);
    manager.put(keys[1], values[1]);

    auto read = manager.ls("/test");
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
    manager.put(keys[0], "0");
    manager.put(keys[1], "1");

    manager.rmdir("/test");

    auto read = manager.keys("/test/");
    BOOST_TEST(read.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(clears_all, fixture) {
    const auto value = std::string("172.0.0.1");
    auto keys = std::vector<std::string>{"/test/a", "/test/b"};
    manager.put(keys[0], "0");
    manager.put(keys[1], "1");

    manager.clear_all();

    auto read = manager.keys("/test/");
    BOOST_TEST(read.size() == 0);
}

BOOST_FIXTURE_TEST_CASE(watches_write, fixture) {
    manager.watch("/test", [&cb = mock.get()](const etcd::Response& response) {
        cb.handle_state_changes(response);
    });

    manager.put("/test/sub/a0", "172.0.0.1");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1);
}

BOOST_FIXTURE_TEST_CASE(watch_rewrite, fixture) {
    manager.put("/test/sub/a0", "172.0.0.1");
    manager.watch("/test", [&cb = mock.get()](const etcd::Response& response) {
        cb.handle_state_changes(response);
    });

    manager.put("/test/sub/a0", "172.0.0.1");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1);
}

BOOST_FIXTURE_TEST_CASE(watches_overwrite, fixture) {
    manager.put("/test/sub/a0", "198.51.100.0");
    manager.watch("/test", [&cb = mock.get()](const etcd::Response& response) {
        cb.handle_state_changes(response);
    });

    manager.put("/test/sub/a0", "172.0.0.1");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1);
}

BOOST_FIXTURE_TEST_CASE(watches_remove, fixture) {
    manager.put("/test/sub/a0", "172.0.0.1");
    manager.watch("/test", [&cb = mock.get()](const etcd::Response& response) {
        cb.handle_state_changes(response);
    });

    manager.rmdir("/test");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1);
}

BOOST_FIXTURE_TEST_CASE(
    returns_lock_guard_and_its_distroyer_doesnt_throw_any_exception, fixture) {
    BOOST_CHECK_NO_THROW(
        { auto lock_guard = manager.get_lock_guard("/foo/bar"); });
}

BOOST_FIXTURE_TEST_CASE(cannot_get_lock_from_same_key_twice, fixture) {
    auto lock_guard = manager.get_lock_guard("/foo/bar");

    BOOST_CHECK_THROW({ auto lock_guard = manager.get_lock_guard("/foo/bar"); },
                      std::invalid_argument);
}
//
// BOOST_FIXTURE_TEST_CASE(
//     can_get_lock_from_same_key_after_first_lock_is_distroyed, fixture) {
//     { auto lock_guard = manager.get_lock_guard("/foo/bar"); }
//
//     BOOST_CHECK_NO_THROW(
//         { auto lock_guard = manager.get_lock_guard("/foo/bar"); });
// }

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
