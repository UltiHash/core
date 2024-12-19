#define BOOST_TEST_MODULE "etcd tests"

#include "common/etcd/utils.h"
#include "fakeit/fakeit.hpp"
#include "utils/system.h"

#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <functional>
#include <thread>

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
    void setup() {}

    fixture() {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~fixture() {}

protected:
    etcd_config cfg;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(MockCallbackTestSuite)

BOOST_FIXTURE_TEST_CASE(watcher_calls_callback_on_key_change, fixture) {
    auto etcd_client = make_etcd_client(cfg);
    etcd_client->set("test0", "initial_value");
    std::shared_ptr<etcd::Watcher> watcher;
    initialize_watcher(
        etcd_client, "test0",
        [&cb = mock.get()](const etcd::Response& response) {
            cb.handle_state_changes(response);
        },
        watcher);

    etcd_client->set("test0", "updated_value");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
}

BOOST_FIXTURE_TEST_CASE(
    watcher_calls_callback_on_key_change_when_it_is_canceled, fixture) {
    auto etcd_client = make_etcd_client(cfg);
    etcd_client->set("test0", "initial_value");
    std::shared_ptr<etcd::Watcher> watcher;
    initialize_watcher(
        etcd_client, "test0",
        [&cb = mock.get()](const etcd::Response& response) {
            cb.handle_state_changes(response);
        },
        watcher);

    watcher->Cancel();
    etcd_client->set("test0", "updated_value");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(0);
}

BOOST_FIXTURE_TEST_CASE(watcher_doesnt_work_when_etcd_stopped, fixture) {

    auto etcd_client = make_etcd_client(cfg);
    etcd_client->set("test0", "initial_value");
    std::shared_ptr<etcd::Watcher> watcher;
    initialize_watcher(
        etcd_client, "test0",
        [&cb = mock.get()](const etcd::Response& response) {
            cb.handle_state_changes(response);
        },
        watcher);

    try {
        BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl stop etcd"), 0);
        std::this_thread::sleep_for(1s);

        etcd_client->set("test0", "updated_value");
        std::this_thread::sleep_for(100ms);

        BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl start etcd"), 0);
        std::this_thread::sleep_for(1s);
    } catch (...) {
        run_with_optional_sudo("systemctl start etcd");
        std::this_thread::sleep_for(1s);
    }

    Verify(Method(mock, handle_state_changes)).Exactly(0_Times);
}

BOOST_FIXTURE_TEST_CASE(watcher_handles_etcd_restart, fixture) {

    auto etcd_client = make_etcd_client(cfg);
    etcd_client->set("test0", "initial_value");
    std::shared_ptr<etcd::Watcher> watcher;
    initialize_watcher(
        etcd_client, "test0",
        [&cb = mock.get()](const etcd::Response& response) {
            cb.handle_state_changes(response);
        },
        watcher);

    try {
        BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl stop etcd"), 0);
        std::this_thread::sleep_for(1s);

        etcd_client->set("test0", "updated_value");
        std::this_thread::sleep_for(100ms);

        BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl start etcd"), 0);
        std::this_thread::sleep_for(1s);

        etcd_client->set("test0", "updated_value");
        std::this_thread::sleep_for(100ms);
    } catch (...) {
        run_with_optional_sudo("systemctl start etcd");
        std::this_thread::sleep_for(1s);
    }

    Verify(Method(mock, handle_state_changes)).Exactly(1_Times);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
