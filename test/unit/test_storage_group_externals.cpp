#define BOOST_TEST_MODULE "storage group state tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <storage/group/externals.h>
#include <storage/group/state_context.h>

namespace uh::cluster::storage {

BOOST_AUTO_TEST_SUITE(a_storage_group_state)

BOOST_AUTO_TEST_CASE(throws_when_theres_no_comma) {
    static constexpr const char* literal = "00102";

    BOOST_CHECK_THROW(storage::group::state_context::create(literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_for_comma_on_wrong_position) {
    static constexpr const char* literal = "00,102";

    BOOST_CHECK_THROW(storage::group::state_context::create(literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_when_group_state_is_out_of_range) {
    static constexpr const char* literal = "9,101";

    BOOST_CHECK_THROW(storage::group::state_context::create(literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(throws_when_storage_state_is_out_of_range) {
    static constexpr const char* literal = "0,103";

    BOOST_CHECK_THROW(storage::group::state_context::create(literal),
                      std::runtime_error);
}

BOOST_AUTO_TEST_CASE(is_well_created) {
    static constexpr const char* literal = "0,102";

    auto context = storage::group::state_context::create(literal);

    BOOST_TEST(static_cast<int>(context.group) == 0);
    BOOST_TEST(context.storages.size() == 3);
    BOOST_TEST(static_cast<int>(context.storages[0]) == 1);
    BOOST_TEST(static_cast<int>(context.storages[1]) == 0);
    BOOST_TEST(static_cast<int>(context.storages[2]) == 2);
}

BOOST_AUTO_TEST_CASE(to_string_is_idempotent) {
    static constexpr const char* literal = "0,102";
    auto context = storage::group::state_context::create(literal);

    auto str = context.to_string();

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

BOOST_FIXTURE_TEST_SUITE(_storage_group_state_context, fixture)

BOOST_AUTO_TEST_CASE(serializes_and_deserializes_well) {
    static constexpr const char* literal = "0,102";
    auto context = deserialize<group::state_context>(literal);
    BOOST_TEST(serialize(context) == literal);
}

BOOST_AUTO_TEST_CASE(is_watched_well) {
    static constexpr const char* literal = "0,102";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto context = deserialize<group::state_context>(literal);
    auto str = context.to_string();
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher =
        storage::group::externals::publisher(etcd, group_id, storage_id);
    auto subscriber = storage::group::externals::subscriber(
        etcd, group_id, num_storages,
        [&](storage::group::state_context*) { p.set_value(); });

    publisher.put_group_state(context);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(subscriber.get_group_state()->to_string() == literal);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_externals, fixture)

BOOST_AUTO_TEST_CASE(subscriber_gets_storage_hostport) {
    static constexpr const char* literal = "localhost:8080";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<hostport>(literal);
    auto str = hp.to_string();
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher =
        storage::group::externals::publisher(etcd, group_id, storage_id);
    auto subscriber = storage::group::externals::subscriber(
        etcd, group_id, num_storages, nullptr,
        [&](hostport*) { p.set_value(); });

    publisher.put_storage_hostport(hp);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(subscriber.get_hostport(storage_id)->to_string() == literal);
}

BOOST_AUTO_TEST_CASE(publisher_destroyes_storage_hostport) {
    static constexpr const char* literal = "localhost:8080";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<hostport>(literal);
    auto str = hp.to_string();
    std::vector<std::promise<void>> promises(2);
    std::vector<std::future<void>> futures;
    for (auto& p : promises) {
        futures.push_back(p.get_future());
    }
    auto callback_count = 0ul;
    auto publisher = std::make_optional<group::externals::publisher>(
        etcd, group_id, storage_id);
    auto subscriber = storage::group::externals::subscriber(
        etcd, group_id, num_storages, nullptr, [&](hostport*) {
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

    BOOST_TEST(subscriber.get_hostport(storage_id)->to_string() == ":0");
}

BOOST_AUTO_TEST_CASE(gets_storage_hostports) {
    static constexpr const char* literal = "localhost:8080";
    auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;
    auto hp = deserialize<hostport>(literal);
    auto str = hp.to_string();
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto publisher =
        storage::group::externals::publisher(etcd, group_id, storage_id);
    auto subscriber = storage::group::externals::subscriber(
        etcd, group_id, num_storages, nullptr,
        [&](hostport*) { p.set_value(); });

    publisher.put_storage_hostport(hp);
    if (f.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto hostports = subscriber.get_hostports();
    BOOST_TEST(hostports[storage_id]->to_string() == literal);
    BOOST_TEST(hostports[0]->to_string() == ":0");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
