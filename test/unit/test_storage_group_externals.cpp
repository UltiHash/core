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
    boost::asio::io_context m_ioc;
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
    auto subscriber =
        externals_subscriber(m_etcd, group_id, num_storages,
                             service_factory<storage_interface>(m_ioc, 2),
                             [&](etcd_manager::response) { p.set_value(); });

    m_etcd.put(ns::root.storage_groups[group_id].storage_states[storage_id],
               serialize(state));
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get_group_state()) == literal);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_externals, fixture)

BOOST_AUTO_TEST_CASE(subscriber_gets_storage_state) {
    static constexpr const char* literal = "1";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto state = deserialize<storage_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto subscriber =
        externals_subscriber(m_etcd, group_id, num_storages,
                             service_factory<storage_interface>(m_ioc, 2),
                             [&](etcd_manager::response) { p.set_value(); });

    m_etcd.put(ns::root.storage_groups[group_id].storage_states[storage_id],
               serialize(state));
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get_storage_states()[storage_id]) ==
               literal);
}

BOOST_AUTO_TEST_CASE(gets_storage_states) {
    static constexpr const char* literal = "1";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto state = deserialize<storage_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto subscriber =
        externals_subscriber(m_etcd, group_id, num_storages,
                             service_factory<storage_interface>(m_ioc, 2),
                             [&](etcd_manager::response) { p.set_value(); });

    m_etcd.put(ns::root.storage_groups[group_id].storage_states[storage_id],
               serialize(state));
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = subscriber.get_storage_states();
    BOOST_TEST(serialize(*states[storage_id]) == literal);
    BOOST_TEST(serialize(*states[0]) == "0");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
