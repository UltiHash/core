#define BOOST_TEST_MODULE "storage state tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <storage/group/internals.h>

namespace uh::cluster::storage {

class callback_interface {
public:
    virtual ~callback_interface() = default;
    virtual void handle_state_changes(etcd_manager::response response) = 0;
};

class fixture {
public:
    fixture()
        : etcd{} {}

    ~fixture() {
        etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    etcd_manager etcd;
};

BOOST_FIXTURE_TEST_SUITE(a_storage_group_state, fixture)

BOOST_AUTO_TEST_CASE(is_created_and_well_detected) {
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher = internals_publisher(etcd, 11, 7);
    auto subscriber = internals_subscriber(
        etcd, 11, 7, [&](etcd_manager::response) { p.set_value(); });

    internals_publisher::set_group_initialized(etcd, 11);

    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*subscriber.get_group_initialized() == true);
}

BOOST_AUTO_TEST_CASE(
    doesnt_delete_group_initialized_on_publishers_destruction) {
    std::vector<std::promise<void>> promises(2);
    std::vector<std::future<void>> futures;
    for (auto& p : promises) {
        futures.push_back(p.get_future());
    }
    auto callback_count = 0ul;
    auto publisher = std::make_optional<internals_publisher>(etcd, 11, 7);
    auto subscriber =
        internals_subscriber(etcd, 11, 7, [&](etcd_manager::response) {
            if (callback_count < promises.size()) {
                promises[callback_count].set_value();
            }
            ++callback_count;
        });
    internals_publisher::set_group_initialized(etcd, 11);
    if (futures[0].wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("First callback was not called within the timeout period");
    }

    publisher.reset();
    BOOST_CHECK(futures[1].wait_for(std::chrono::seconds(5)) ==
                std::future_status::timeout);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_internals, fixture)

BOOST_AUTO_TEST_CASE(subscriber_gets_storage_state) {
    static constexpr const char* literal = "1";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<storage_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher = internals_publisher(etcd, group_id, storage_id);
    auto subscriber =
        internals_subscriber(etcd, group_id, num_storages,
                             [&](etcd_manager::response) { p.set_value(); });

    publisher.put_storage_state(hp);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get_storage_states()[storage_id]) ==
               literal);
}

BOOST_AUTO_TEST_CASE(publisher_destroyes_storage_state) {
    static constexpr const char* literal = "1";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<storage_state>(literal);
    std::vector<std::promise<void>> promises(2);
    std::vector<std::future<void>> futures;
    for (auto& p : promises) {
        futures.push_back(p.get_future());
    }
    auto callback_count = 0ul;
    auto publisher =
        std::make_optional<internals_publisher>(etcd, group_id, storage_id);
    auto subscriber = internals_subscriber(
        etcd, group_id, num_storages, [&](etcd_manager::response) {
            if (callback_count < promises.size()) {
                promises[callback_count].set_value();
            }
            ++callback_count;
        });
    publisher->put_storage_state(hp);
    if (futures[0].wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("First callback was not called within the timeout period");
    }

    publisher.reset();
    if (futures[1].wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("First callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get_storage_states()[storage_id]) == "0");
}

BOOST_AUTO_TEST_CASE(gets_storage_states) {
    static constexpr const char* literal = "2";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<storage_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher = internals_publisher(etcd, group_id, storage_id);
    auto subscriber =
        internals_subscriber(etcd, group_id, num_storages,
                             [&](etcd_manager::response) { p.set_value(); });

    publisher.put_storage_state(hp);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = subscriber.get_storage_states();
    BOOST_TEST(serialize(*states[storage_id]) == literal);
    BOOST_TEST(serialize(*states[0]) == "0");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
