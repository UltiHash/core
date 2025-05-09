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
    BOOST_TEST(group_initialized_manager::get(etcd, 11) == false);
}

BOOST_AUTO_TEST_CASE(is_created_and_well_detected) {
    group_initialized_manager::put_persistant(etcd, 11, true);

    BOOST_TEST(group_initialized_manager::get(etcd, 11) == true);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_internals, fixture)

BOOST_AUTO_TEST_CASE(subscriber_gets_storage_state) {
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    service_config service_cfg;
    std::promise<void> p;
    std::future<void> f = p.get_future();
    temp_directory tmp_dir;
    auto prefix = ns::root.storage_groups[group_id].storage_states[storage_id];
    auto storage_states = vector_observer<storage_state>(prefix, num_storages);
    auto sub = subscriber("test subscriber", etcd, prefix, {storage_states},
                          [&]() { p.set_value(); });

    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(serialize(*subscriber.storage_states().get()[storage_id]) ==
               "1");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
