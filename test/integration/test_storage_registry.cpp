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

class basic_fixture {
public:
    basic_fixture()
        : m_etcd{} {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

    virtual ~basic_fixture() {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    const std::size_t m_num_storages = 7;
    const std::size_t m_service_id = 3;
    const std::size_t m_group_id = 4;
    etcd_manager m_etcd;
    temp_directory m_tmp_dir;
};

class fixture_with_subscriber : public basic_fixture {
public:
    fixture_with_subscriber()
        : basic_fixture{} {}
    ~fixture_with_subscriber() {}

    bool wait_for_storage_states_key() {
        std::unique_lock<std::mutex> lock(cv_mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(2),
                         [&] { return storage_states_updated; })) {
            return false;
        }
        storage_states_updated = false;
        return true;
    }

protected:
    std::condition_variable cv;
    std::mutex cv_mutex;
    bool storage_states_updated = false;
    vector_observer<storage_state> m_storage_states_observer{
        ns::root.storage_groups[m_group_id].storage_states,
        m_num_storages,
        {},
        [&](std::size_t id, storage_state& state) {
            std::lock_guard<std::mutex> lock(cv_mutex);
            storage_states_updated = true;
            cv.notify_one();
        }};
    subscriber m_subscriber{"fixture",
                            m_etcd,
                            ns::root.storage_groups[m_group_id],
                            {m_storage_states_observer}};
};

BOOST_FIXTURE_TEST_SUITE(a_storage_registry, fixture_with_subscriber)

BOOST_AUTO_TEST_CASE(registers_current_storage_state_as_new) {
    auto sut =
        storage_registry(m_etcd, m_group_id, m_service_id, m_tmp_dir.path());

    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = m_storage_states_observer.get();
    BOOST_CHECK(*states[3] == storage_state::NEW);
    BOOST_CHECK(*states[2] == storage_state::DOWN);
}

BOOST_AUTO_TEST_CASE(supports_updating_state_to_assigned) {
    auto sut =
        storage_registry(m_etcd, m_group_id, m_service_id, m_tmp_dir.path());
    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    sut.set(storage_state::ASSIGNED);

    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = m_storage_states_observer.get();
    BOOST_CHECK(*states[3] == storage_state::ASSIGNED);
    BOOST_CHECK(*states[2] == storage_state::DOWN);
}

BOOST_AUTO_TEST_CASE(clears_etcd_key_when_destroyed) {
    auto sut = std::make_optional<storage_registry>(
        m_etcd, m_group_id, m_service_id, m_tmp_dir.path());
    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    sut->set(storage_state::ASSIGNED);
    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    sut.reset();

    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = m_storage_states_observer.get();
    BOOST_CHECK(*states[3] == storage_state::DOWN);
    BOOST_CHECK(*states[2] == storage_state::DOWN);
}

BOOST_AUTO_TEST_CASE(restores_previous_state) {
    auto sut = std::make_optional<storage_registry>(
        m_etcd, m_group_id, m_service_id, m_tmp_dir.path());
    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    sut->set(storage_state::ASSIGNED);
    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    sut.reset();
    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    auto sut_2 =
        storage_registry(m_etcd, m_group_id, m_service_id, m_tmp_dir.path());

    if (!wait_for_storage_states_key()) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto states = m_storage_states_observer.get();
    BOOST_CHECK(*states[3] == storage_state::ASSIGNED);
    BOOST_CHECK(*states[2] == storage_state::DOWN);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
