#include "storage/group/internals.h"
#include <boost/test/tools/old/interface.hpp>
#include <thread>
#define BOOST_TEST_MODULE "storage_registry tests"

#include "common/etcd/namespace.h"
#include "common/etcd/utils.h"
#include <boost/asio/ip/host_name.hpp>
#include <boost/test/unit_test.hpp>
#include <common/etcd/registry/storage_registry.h>
#include <common/service_interfaces/hostport.h>
#include <common/utils/strings.h>
#include <util/temp_directory.h>

// ------------- Tests Suites Follow --------------

namespace uh::cluster::storage {

BOOST_AUTO_TEST_SUITE(a_storage_registry)

BOOST_AUTO_TEST_CASE(registers_current_storage_state_as_new) {
    const auto num_storages = 7;
    const auto service_id = 3;
    const auto group_id = 4;
    auto etcd = etcd_manager();
    temp_directory tmp_dir;
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    storage_state_subscriber subscriber(
        etcd, group_id, num_storages,
        [&](std::size_t, storage_state) { promise.set_value(); });

    auto sut = storage_registry(etcd, group_id, service_id, tmp_dir.path());

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = subscriber.get();
    BOOST_CHECK(*states[3] == storage_state::NEW);
    BOOST_CHECK(*states[2] == storage_state::DOWN);
}

BOOST_AUTO_TEST_CASE(supports_updating_state_to_assigned) {
    const auto num_storages = 7;
    const auto service_id = 3;
    const auto group_id = 4;
    auto etcd = etcd_manager();
    temp_directory tmp_dir;
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    storage_state_subscriber subscriber(
        etcd, group_id, num_storages,
        [&](std::size_t id, storage_state& state) {
            if (id == 3 && state == storage_state::ASSIGNED) {
                promise.set_value();
            }
        });
    auto sut = storage_registry(etcd, group_id, service_id, tmp_dir.path());

    sut.set(storage_state::ASSIGNED);
    sut.publish();

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = subscriber.get();
    BOOST_CHECK(*states[3] == storage_state::ASSIGNED);
    BOOST_CHECK(*states[2] == storage_state::DOWN);
}

BOOST_AUTO_TEST_CASE(clears_etcd_key_when_destroyed) {
    const auto num_storages = 7;
    const auto service_id = 3;
    const auto group_id = 4;
    auto etcd = etcd_manager();
    temp_directory tmp_dir;
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    auto sut = std::make_optional<storage_registry>(etcd, group_id, service_id,
                                                    tmp_dir.path());
    sut->set(storage_state::ASSIGNED);
    sut->publish();
    storage_state_subscriber subscriber(
        etcd, group_id, num_storages,
        [&](std::size_t id, storage_state& state) {
            if (state == storage_state::DOWN)
                promise.set_value();
        });

    sut.reset();

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = subscriber.get();
    BOOST_CHECK(*states[3] == storage_state::DOWN);
    BOOST_CHECK(*states[2] == storage_state::DOWN);
}

BOOST_AUTO_TEST_CASE(restores_previous_state) {
    const auto num_storages = 7;
    const auto service_id = 3;
    const auto group_id = 4;
    auto etcd = etcd_manager();
    temp_directory tmp_dir;
    auto promise = std::promise<void>();
    auto future = promise.get_future();
    auto sut = std::make_optional<storage_registry>(etcd, group_id, service_id,
                                                    tmp_dir.path());
    sut->set(storage_state::ASSIGNED);
    sut->publish();
    sut.reset();

    storage_state_subscriber subscriber(
        etcd, group_id, num_storages,
        [&](std::size_t id, storage_state& state) { promise.set_value(); });

    auto sut_2 = storage_registry(etcd, group_id, service_id, tmp_dir.path());

    if (future.wait_for(std::chrono::seconds(5)) ==
        std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = subscriber.get();
    BOOST_CHECK(*states[3] == storage_state::ASSIGNED);
    BOOST_CHECK(*states[2] == storage_state::DOWN);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
