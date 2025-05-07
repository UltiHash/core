#define BOOST_TEST_MODULE "storage state tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <storage/group/internals.h>
#include <util/temp_directory.h>

namespace uh::cluster::storage {

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

BOOST_AUTO_TEST_CASE(reads_false_by_default) {
    BOOST_TEST(etcd_group_initialized::get(etcd, 11) == false);
}

BOOST_AUTO_TEST_CASE(is_created_and_well_detected) {
    etcd_group_initialized::put(etcd, 11, true);

    BOOST_TEST(etcd_group_initialized::get(etcd, 11) == true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_internals, fixture)

BOOST_AUTO_TEST_CASE(subscriber_gets_storage_state) {
    static constexpr const char* literal = "1";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto state = deserialize<storage_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    temp_directory tmp_dir;
    auto subscriber = storage_state_subscriber(etcd, group_id, num_storages,
                                               [&]() { p.set_value(); });

    etcd.put(ns::root.storage_groups[group_id].storage_states[storage_id],
             serialize(state));
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.get()[storage_id]) == literal);
}

BOOST_AUTO_TEST_CASE(gets_storage_states) {
    static constexpr const char* literal = "2";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto state = deserialize<storage_state>(literal);
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto subscriber = storage_state_subscriber(etcd, group_id, num_storages,
                                               [&]() { p.set_value(); });

    etcd.put(ns::root.storage_groups[group_id].storage_states[storage_id],
             serialize(state));
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = subscriber.get();
    BOOST_TEST(serialize(*states[storage_id]) == literal);
    BOOST_TEST(serialize(*states[0]) == "0");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
