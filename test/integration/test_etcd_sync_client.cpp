#define BOOST_TEST_MODULE "etcd tests"

#include <etcd/KeepAlive.hpp>
#include <etcd/SyncClient.hpp>
#include <etcd/Watcher.hpp>

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

    void print_ls() {
        auto resp = etcd_client.ls("/");
        auto keys = resp.keys();
        std::map<std::string, std::string> map;
        for (auto i = 0u; i < keys.size(); ++i) {
            map[keys[i]] = resp.value(i).as_string();
        }
        for (const auto& [key, value] : map) {
            std::cout << key << ": " << value << std::endl;
        }
    }

protected:
    std::string etcd_address;
    etcd::SyncClient etcd_client;
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

BOOST_FIXTURE_TEST_CASE(campaigns_and_observes, fixture) {
    auto keepalive = etcd_client.leasekeepalive(60);
    auto lease_id = keepalive->Lease();

    auto observer_thread = std::thread([&]() {
        std::unique_ptr<etcd::SyncClient::Observer> observer =
            etcd_client.observe("test");

        // wait many change events, blocked execution
        for (size_t i = 0; i < 10; ++i) {
            etcd::Response resp = observer->WaitOnce();
            std::cout << "observe " << resp.value().key()
                      << " as the leader: " << resp.value().as_string()
                      << std::endl;
        }
        std::cout << "finish the observe" << std::endl;
        // cancel the observers
        observer.reset(nullptr);
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    for (int i = 0; i < 5; ++i) {
        // campaign
        auto resp1 = etcd_client.campaign("test", lease_id, "xxxx");
        BOOST_TEST(0 == resp1.error_code());
        std::cout << "key " << resp1.value().key() << " becomes the leader"
                  << std::endl;

        // proclaim
        auto resp3 = etcd_client.proclaim("test", lease_id, resp1.value().key(),
                                          resp1.value().created_index(),
                                          "tttt - " + std::to_string(i));
        BOOST_TEST(0 == resp3.error_code());

        // leader
        {
            auto resp4 = etcd_client.leader("test");
            BOOST_TEST(0 == resp4.error_code());
            BOOST_TEST(resp1.value().key() == resp4.value().key());
            BOOST_TEST("tttt - " + std::to_string(i) ==
                       resp4.value().as_string());
        }

        // resign
        auto resp5 = etcd_client.resign("test", lease_id, resp1.value().key(),
                                        resp1.value().created_index());
        BOOST_TEST(0 == resp5.error_code());
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    observer_thread.join();
}

BOOST_FIXTURE_TEST_CASE(concurrent_campaign, fixture) {
    auto keepalive = etcd_client.leasekeepalive(60);
    auto lease_id = keepalive->Lease();

    auto resp1 = etcd_client.campaign("test", lease_id, "1");
    BOOST_TEST(0 == resp1.error_code());
    std::clog << "key " << resp1.value().key() << " becomes the leader"
              << std::endl;

    auto observer_thread = std::thread([&]() {
        auto keepalive = etcd_client.leasekeepalive(60);
        auto lease_id = keepalive->Lease();
        auto resp1 = etcd_client.campaign("test", lease_id, "1");
        BOOST_TEST(0 == resp1.error_code());
        std::clog << "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@" << std::endl;
        std::clog << "Waiting follower becomes the leader" << std::endl;
        std::clog << "key " << resp1.value().key() << " becomes the leader"
                  << std::endl;
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // resign
    auto resp2 = etcd_client.resign("test", lease_id, resp1.value().key(),
                                    resp1.value().created_index());
    BOOST_TEST(0 == resp2.error_code());

    std::this_thread::sleep_for(std::chrono::seconds(1));

    observer_thread.join();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
