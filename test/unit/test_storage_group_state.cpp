#define BOOST_TEST_MODULE "storage group state tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <storage/group/state.h>
#include <storage/group/state_watcher.h>

using namespace uh::cluster;

BOOST_AUTO_TEST_SUITE(a_storage_group_state)

BOOST_AUTO_TEST_CASE(throws_when_theres_no_comma) {
    static constexpr const char* literal = "00102";

    BOOST_CHECK_THROW(storage_group::state::create(literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_for_comma_on_wrong_position) {
    static constexpr const char* literal = "00,102";

    BOOST_CHECK_THROW(storage_group::state::create(literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_when_group_state_is_out_of_range) {
    static constexpr const char* literal = "9,101";

    BOOST_CHECK_THROW(storage_group::state::create(literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_when_storage_state_is_out_of_range) {
    static constexpr const char* literal = "0,103";

    BOOST_CHECK_THROW(storage_group::state::create(literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(is_well_created) {
    static constexpr const char* literal = "0,102";

    auto state = storage_group::state::create(literal);

    BOOST_TEST(static_cast<int>(state.group) == 0);
    BOOST_TEST(state.storages.size() == 3);
    BOOST_TEST(static_cast<int>(state.storages[0]) == 1);
    BOOST_TEST(static_cast<int>(state.storages[1]) == 0);
    BOOST_TEST(static_cast<int>(state.storages[2]) == 2);
}

BOOST_AUTO_TEST_CASE(to_string_is_idempotent) {
    static constexpr const char* literal = "0,102";
    auto state = storage_group::state::create(literal);

    auto str = state.to_string();

    BOOST_TEST(str == literal);
}

BOOST_AUTO_TEST_SUITE_END()

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

BOOST_AUTO_TEST_CASE(is_watched_well) {
    static constexpr const char* literal = "0,102";
    auto state = storage_group::state::create(literal);
    auto str = state.to_string();
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto watcher = storage_group::state_watcher(
        etcd, 11, [&](storage_group::state*) { p.set_value(); });

    etcd.put(get_storage_group_state_path(11), str);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(watcher.get_state()->to_string() == literal);
}

BOOST_AUTO_TEST_SUITE_END()
