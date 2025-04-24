#define BOOST_TEST_MODULE "storage group state tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <storage/group/externals.h>
#include <storage/group/state.h>

namespace uh::cluster::storage {

class fixture {
public:
    fixture()
        : m_etcd{} {}

    ~fixture() {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    etcd_manager m_etcd;
};

BOOST_FIXTURE_TEST_SUITE(a_storage_group_state, fixture)

BOOST_AUTO_TEST_CASE(serializes_and_deserializes_well) {
    static constexpr const char* literal = "0";
    auto state = deserialize<group_state>(literal);
    BOOST_TEST(serialize(state) == literal);
}

BOOST_AUTO_TEST_CASE(is_watched_well) {
    static constexpr const char* literal = "0";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto state = deserialize<group_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher = externals_publisher(m_etcd, group_id, storage_id);
    auto subscriber =
        externals_subscriber(m_etcd, group_id, num_storages,
                             [&](etcd_manager::response) { p.set_value(); });

    publisher.put_group_state(state);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get_group_state()) == literal);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_externals, fixture)

BOOST_AUTO_TEST_CASE(subscriber_gets_storage_hostport) {
    static constexpr const char* literal = "localhost:8080";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<hostport>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher = externals_publisher(m_etcd, group_id, storage_id);
    auto subscriber =
        externals_subscriber(m_etcd, group_id, num_storages,
                             [&](etcd_manager::response) { p.set_value(); });

    publisher.put_storage_hostport(hp);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get_storage_hostports()[storage_id]) ==
               literal);
}

BOOST_AUTO_TEST_CASE(publisher_destroyes_storage_hostport) {
    static constexpr const char* literal = "localhost:8080";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<hostport>(literal);
    std::vector<std::promise<void>> promises(2);
    std::vector<std::future<void>> futures;
    for (auto& p : promises) {
        futures.push_back(p.get_future());
    }
    auto callback_count = 0ul;
    auto publisher =
        std::make_optional<externals_publisher>(m_etcd, group_id, storage_id);
    auto subscriber = externals_subscriber(
        m_etcd, group_id, num_storages, [&](etcd_manager::response) {
            if (callback_count < promises.size()) {
                promises[callback_count].set_value();
            }
            ++callback_count;
        });
    publisher->put_storage_hostport(hp);
    if (futures[0].wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("First callback was not called within the timeout period");
    }

    publisher.reset();
    if (futures[1].wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("First callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get_storage_hostports()[storage_id]) ==
               ":0");
}

BOOST_AUTO_TEST_CASE(gets_storage_hostports) {
    static constexpr const char* literal = "localhost:8080";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<hostport>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher = externals_publisher(m_etcd, group_id, storage_id);
    auto subscriber =
        externals_subscriber(m_etcd, group_id, num_storages,
                             [&](etcd_manager::response) { p.set_value(); });

    publisher.put_storage_hostport(hp);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto hostports = subscriber.get_storage_hostports();
    BOOST_TEST(serialize(*hostports[storage_id]) == literal);
    BOOST_TEST(serialize(*hostports[0]) == ":0");
}

BOOST_AUTO_TEST_CASE(subscriber_gets_storage_state) {
    static constexpr const char* literal = "1";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<storage_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher = externals_publisher(m_etcd, group_id, storage_id);
    auto subscriber =
        externals_subscriber(m_etcd, group_id, num_storages,
                             [&](etcd_manager::response) { p.set_value(); });

    publisher.put_storage_state(hp);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get_storage_states()[storage_id]) ==
               literal);
}

BOOST_AUTO_TEST_CASE(publisher_destroyes_storage_state) {
    static constexpr const char* literal = "2";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<storage_state>(literal);
    std::vector<std::promise<void>> promises(2);
    std::vector<std::future<void>> futures;
    for (auto& p : promises) {
        futures.push_back(p.get_future());
    }
    auto callback_count = 0ul;
    auto publisher =
        std::make_optional<externals_publisher>(m_etcd, group_id, storage_id);
    auto subscriber = externals_subscriber(
        m_etcd, group_id, num_storages, [&](etcd_manager::response) {
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
    static constexpr const char* literal = "1";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<storage_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher = externals_publisher(m_etcd, group_id, storage_id);
    auto subscriber =
        externals_subscriber(m_etcd, group_id, num_storages,
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
