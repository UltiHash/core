#define BOOST_TEST_MODULE "etcd tests"

#include "common/etcd/utils.h"
#include "fakeit.hpp"

#include <boost/test/unit_test.hpp>
#include <future>

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
    fixture()
        : etcd_address{"http://127.0.0.1:2379"},
          etcd_client{etcd_address} {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~fixture() { etcd_client.rmdir("/", true); }

protected:
    std::string etcd_address;
    etcd::SyncClient etcd_client;
    etcd_config cfg;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(a_etcd_client)

BOOST_FIXTURE_TEST_CASE(watches_changes_on_the_given_key, fixture) {
    etcd_client.put("test0", "initial_value");
    std::shared_ptr<etcd::Watcher> watcher;
    watcher.reset(new etcd::Watcher(
        etcd_client, "test0",
        [&cb = mock.get()](const etcd::Response& response) {
            cb.handle_state_changes(response);
        },
        true /*recursive*/));

    etcd_client.put("test0", "updated_value");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
}

BOOST_FIXTURE_TEST_CASE(
    cannot_watch_changes_on_the_given_key_after_cancellation, fixture) {
    etcd_client.put("test0", "initial_value");
    std::shared_ptr<etcd::Watcher> watcher;
    watcher.reset(new etcd::Watcher(
        etcd_client, "test0",
        [&cb = mock.get()](const etcd::Response& response) {
            cb.handle_state_changes(response);
        },
        true /*recursive*/));

    watcher->Cancel();
    etcd_client.put("test0", "updated_value");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(0);
}

BOOST_FIXTURE_TEST_CASE(reads_written_value, fixture) {
    etcd_client.put("/foo/bar", "1");

    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.value().as_string() == "1");
}

BOOST_FIXTURE_TEST_CASE(gets_leasegrant, fixture) {
    auto resp = etcd_client.leasegrant(2);

    BOOST_TEST(resp.is_ok() == true);
}

BOOST_FIXTURE_TEST_CASE(cannot_read_after_lease_expired, fixture) {
    auto lease = etcd_client.leasegrant(2).value().lease();
    etcd_client.put("/foo/bar", "1", lease);

    std::this_thread::sleep_for(4s);
    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.is_ok() == false);
}

BOOST_FIXTURE_TEST_CASE(fails_getting_lock_when_lease_is_invalidated, fixture) {
    auto lease = etcd_client.leasegrant(2).value().lease();
    auto keepalive = etcd::KeepAlive(etcd_client, 10, lease);
    auto key = std::string("/foo/bar");
    std::this_thread::sleep_for(3s);

    auto resp = etcd_client.lock_with_lease(key, lease);

    BOOST_TEST(resp.is_ok() == false);
}

BOOST_FIXTURE_TEST_CASE(succeeds_getting_lock_with_valid_lease, fixture) {
    auto lease = etcd_client.leasegrant(2).value().lease();
    auto keepalive = etcd::KeepAlive(etcd_client, 1, lease);
    auto key = std::string("/foo/bar");
    std::this_thread::sleep_for(3s);

    auto resp = etcd_client.lock_with_lease(key, lease);

    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.lock_key().substr(0, key.size()) == key);
}

BOOST_FIXTURE_TEST_CASE(succeeds_unlocking_with_key_given_from_locking,
                        fixture) {
    auto lease = etcd_client.leasegrant(2).value().lease();
    auto keepalive = etcd::KeepAlive(etcd_client, 1, lease);
    auto key = std::string("/foo/bar");
    auto resp_lock = etcd_client.lock_with_lease(key, lease);
    std::this_thread::sleep_for(3s);

    auto resp_unlock = etcd_client.unlock(resp_lock.lock_key());

    BOOST_TEST(resp_unlock.is_ok() == true);
}

BOOST_FIXTURE_TEST_CASE(waits_on_second_lock_until_first_lock_is_unlocked,
                        fixture) {

    auto lease = etcd_client.leasegrant(30).value().lease();
    auto key = std::string("/foo/bar");
    auto resp_lock = etcd_client.lock_with_lease(key, lease);

    std::future<etcd::Response> future_lock2 =
        std::async(std::launch::async, [&]() {
            auto lease = etcd_client.leasegrant(30).value().lease();
            return etcd_client.lock_with_lease(key, lease);
        });

    std::this_thread::sleep_for(std::chrono::seconds(2));
    BOOST_CHECK(future_lock2.wait_for(std::chrono::seconds(0)) ==
                std::future_status::timeout);

    auto resp_unlock = etcd_client.unlock(resp_lock.lock_key());
    BOOST_TEST(resp_unlock.is_ok());
    auto resp_lock2 = future_lock2.get();
    BOOST_TEST(resp_lock2.is_ok());
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
