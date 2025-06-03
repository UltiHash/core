#define BOOST_TEST_MODULE "etcd tests"

#include <etcd/KeepAlive.hpp>
#include <etcd/SyncClient.hpp>
#include <etcd/Watcher.hpp>
#include <etcd/v3/Transaction.hpp>

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
            .AlwaysDo([&](const etcd::Response& resp) { watch_resp = resp; });
        etcd_client.rmdir("/", true);
        std::this_thread::sleep_for(100ms);
    }

    ~fixture() {
        etcd_client.rmdir("/", true);
        watch_resp = etcd::Response();
    }

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
    etcd::Response watch_resp;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(get)

BOOST_FIXTURE_TEST_CASE(returns_written_value, fixture) {
    etcd_client.put("/foo/bar", "1");

    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.value().as_string() == "1");
}

BOOST_FIXTURE_TEST_CASE(returns_correct_action_and_index_for_get, fixture) {
    etcd_client.put("/foo/bar", "1");
    auto prev_resp = etcd_client.get("/foo/bar");
    auto prev_idx = prev_resp.index();

    etcd_client.put("/foo/bar", "1");
    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.action() == "get");
    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.index() == prev_idx + 1);
    BOOST_TEST(resp.value().modified_index() == prev_idx + 1);
    BOOST_TEST(resp.value().as_string() == "1");
}

BOOST_FIXTURE_TEST_CASE(returns_correct_index_after_rm, fixture) {
    etcd_client.put("/foo/bar", "1");
    auto prev_resp = etcd_client.get("/foo/bar");
    auto prev_idx = prev_resp.index();

    etcd_client.rm("/foo/bar");
    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.action() == "get");
    BOOST_TEST(resp.is_ok() == false);
    BOOST_TEST(resp.index() == prev_idx + 1);
    BOOST_TEST(resp.value().modified_index() == 0);
    BOOST_TEST(resp.value().as_string() == "");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(ls)

BOOST_FIXTURE_TEST_CASE(returns_correct_action_and_index, fixture) {
    etcd_client.put("/foo/bar", "1");
    auto pref_resp = etcd_client.ls("/foo");
    auto prev_idx = pref_resp.index();

    etcd_client.put("/foo/bar2", "2");
    auto resp = etcd_client.ls("/foo");

    BOOST_TEST(resp.action() == "get");
    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.index() == prev_idx + 1);
    BOOST_TEST(resp.values()[0].key() == "/foo/bar");
    BOOST_TEST(resp.values()[0].as_string() == "1");
    BOOST_TEST(resp.values()[1].key() == "/foo/bar2");
    BOOST_TEST(resp.values()[1].as_string() == "2");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(a_watcher)

BOOST_FIXTURE_TEST_CASE(watches_creation, fixture) {
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    auto watcher = etcd::Watcher(
        etcd_client, "/test0",
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    etcd_client.put("/test0", "initial_value");
    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "create");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "initial_value");
}

BOOST_FIXTURE_TEST_CASE(watches_creation_with_0_index, fixture) {
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    auto watcher = etcd::Watcher(
        etcd_client, "/test0", 0,
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    etcd_client.put("/test0", "initial_value");
    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "create");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "initial_value");
}

BOOST_FIXTURE_TEST_CASE(watches_changes_on_the_given_key, fixture) {
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    etcd_client.put("/test0", "initial_value");
    auto watcher = etcd::Watcher(
        etcd_client, "/test0",
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    etcd_client.put("/test0", "updated_value");
    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "set");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "updated_value");
}

BOOST_FIXTURE_TEST_CASE(watches_deletion, fixture) {
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    etcd_client.put("/test0", "initial_value");
    auto watcher = etcd::Watcher(
        etcd_client, "/test0",
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    etcd_client.rm("/test0");
    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "delete");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "");
}

BOOST_FIXTURE_TEST_CASE(watches_deletion_with_index_0, fixture) {
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    etcd_client.put("/test0", "initial_value");
    auto watcher = etcd::Watcher(
        etcd_client, "/test0", 0,
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    etcd_client.rm("/test0");
    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "delete");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "");
}

BOOST_FIXTURE_TEST_CASE(watches_deletion_with_previndex_plus_1, fixture) {
    auto lease_id = etcd_client.leasegrant(1).value().lease();
    auto resp = etcd_client.put("/test0", "initial_value", lease_id);
    auto promise = std::promise<void>();
    auto future = promise.get_future();

    // etcd::SyncClient etcd_client_for_watcher;
    auto watcher = etcd::Watcher(
        etcd_client, "/test0", resp.index() + 1,
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    auto get_resp = etcd_client.get("/test0");
    BOOST_TEST(get_resp.is_ok() == false);

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "delete");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "");
}

BOOST_FIXTURE_TEST_CASE(
    cannot_watch_changes_on_the_given_key_after_cancellation, fixture) {
    etcd_client.put("/test0", "initial_value");
    auto watcher = etcd::Watcher(
        etcd_client, "/test0",
        [&cb = mock.get()](const etcd::Response& response) {
            cb.handle_state_changes(response);
        },
        true /*recursive*/);

    watcher.Cancel();
    etcd_client.put("/test0", "updated_value");
    std::this_thread::sleep_for(200ms);

    Verify(Method(mock, handle_state_changes)).Exactly(0);
}

BOOST_FIXTURE_TEST_CASE(cannot_watch_changes_occured_before_creating_watcher,
                        fixture) {
    etcd_client.put("/test0", "initial_value");

    auto watcher = etcd::Watcher(
        etcd_client, "/test0",
        [&cb = mock.get()](const etcd::Response& response) {
            cb.handle_state_changes(response);
        },
        true /*recursive*/);
    std::this_thread::sleep_for(200ms);

    Verify(Method(mock, handle_state_changes)).Exactly(0);
}

BOOST_FIXTURE_TEST_CASE(watches_changes_previous_watcher_changes_using_index,
                        fixture) {
    etcd_client.put("/test0", "initial_value");
    auto get_resp = etcd_client.get("/test0");
    etcd_client.put("/test0", "second_value");
    auto promise = std::promise<void>();
    auto future = promise.get_future();

    auto watcher = etcd::Watcher(
        etcd_client, "/test0", get_resp.index() + 1,
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "set");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "second_value");
}

BOOST_FIXTURE_TEST_CASE(
    watches_changes_recursively_previous_watcher_changes_using_index, fixture) {
    etcd_client.put("/test/0", "val_0");
    auto ls_resp = etcd_client.ls("/test");
    etcd_client.put("/test/1", "val_1");
    etcd_client.put("/test/2", "val_2");
    etcd_client.rm("/test/2");

    auto promise = std::promise<void>();
    auto future = promise.get_future();

    auto watcher = etcd::Watcher(
        etcd_client, "/test", ls_resp.index() + 1,
        [&promise](const etcd::Response& resp) {
            BOOST_TEST(resp.action(0) == "create");
            BOOST_TEST(resp.value(0).key() == "/test/1");
            BOOST_TEST(resp.value(0).as_string() == "val_1");
            BOOST_TEST(resp.action(1) == "create");
            BOOST_TEST(resp.value(1).key() == "/test/2");
            BOOST_TEST(resp.value(1).as_string() == "val_2");
            BOOST_TEST(resp.action(2) == "delete");
            BOOST_TEST(resp.value(2).key() == "/test/2");
            BOOST_TEST(resp.value(2).as_string() == "");
            promise.set_value();
        },
        true /*recursive*/);

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(modify_if)

BOOST_FIXTURE_TEST_CASE(with_oldindex_0_doesnt_update_when_theres_key,
                        fixture) {
    etcd_client.modify_if("/foo/bar", "1", 0);

    auto resp_modify_if = etcd_client.modify_if("/foo/bar", "2", 0);
    auto resp_get = etcd_client.get("/foo/bar");

    BOOST_TEST(resp_modify_if.is_ok() == false);
    BOOST_TEST(resp_modify_if.error_code() == etcdv3::ERROR_COMPARE_FAILED);
    BOOST_TEST(resp_get.action() == "get");
    BOOST_TEST(resp_get.is_ok() == true);
    BOOST_TEST(resp_get.value().as_string() == "1");
}

BOOST_FIXTURE_TEST_CASE(with_oldindex_0_updates_when_theres_no_key, fixture) {
    etcd_client.modify_if("/foo/bar", "1", 0);
    etcd_client.rm("/foo/bar");
    etcd_client.modify_if("/foo/bar", "2", 0);

    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.action() == "get");
    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.value().as_string() == "2");
}

BOOST_FIXTURE_TEST_CASE(with_watcher_with_previndex_plus_1_watches_key_deletion,
                        fixture) {

    auto lease_id = etcd_client.leasegrant(1).value().lease();
    auto resp = etcd_client.modify_if("/test0", "initial_value", 0, lease_id);
    auto promise = std::promise<void>();
    auto future = promise.get_future();

    // etcd::SyncClient etcd_client_for_watcher;
    auto watcher = etcd::Watcher(
        etcd_client, "/test0", resp.index() + 1,
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    auto get_resp = etcd_client.get("/test0");
    BOOST_TEST(get_resp.is_ok() == false);

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "delete");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(add)

BOOST_FIXTURE_TEST_CASE(updates_even_though_theres_key, fixture) {
    etcd_client.add("/foo/bar", "1");

    auto resp_modify_if = etcd_client.add("/foo/bar", "2");
    auto resp_get = etcd_client.get("/foo/bar");

    BOOST_TEST(resp_modify_if.is_ok() == false);
    BOOST_TEST(resp_modify_if.error_code() == etcdv3::ERROR_KEY_ALREADY_EXISTS);
    BOOST_TEST(resp_get.action() == "get");
    BOOST_TEST(resp_get.is_ok() == true);
    // It's not same as https://github.com/etcd-cpp-apiv3/etcd-cpp-apiv3
    // describes
    BOOST_TEST(resp_get.value().as_string() == "2");
}

BOOST_FIXTURE_TEST_CASE(with_oldindex_0_updates_when_theres_no_key, fixture) {
    etcd_client.add("/foo/bar", "1");
    etcd_client.rm("/foo/bar");
    etcd_client.add("/foo/bar", "2");

    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.action() == "get");
    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.value().as_string() == "2");
}

BOOST_FIXTURE_TEST_CASE(with_watcher_with_previndex_plus_1_watches_key_deletion,
                        fixture) {

    auto lease_id = etcd_client.leasegrant(1).value().lease();
    auto resp = etcd_client.add("/test0", "initial_value", lease_id);
    auto promise = std::promise<void>();
    auto future = promise.get_future();

    // etcd::SyncClient etcd_client_for_watcher;
    auto watcher = etcd::Watcher(
        etcd_client, "/test0", resp.index() + 1,
        [&cb = mock.get(), &promise](const etcd::Response& response) {
            cb.handle_state_changes(response);
            promise.set_value();
        },
        true /*recursive*/);

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    auto get_resp = etcd_client.get("/test0");
    BOOST_TEST(get_resp.is_ok() == false);

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
    BOOST_TEST(watch_resp.action() == "delete");
    BOOST_TEST(watch_resp.value().key() == "/test0");
    BOOST_TEST(watch_resp.value().as_string() == "");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(a_etcd_client)

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
