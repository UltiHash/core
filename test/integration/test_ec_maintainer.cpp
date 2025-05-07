#define BOOST_TEST_MODULE "ec_maintainer tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>
#include <format>
#include <storage/group/ec_maintainer.h>
#include <storage/group/externals.h>
#include <storage/group/state.h>
#include <util/temp_directory.h>

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
    const std::size_t m_num_instances = 3;
    group_config m_group_cfg{.id = 0,
                             .type = group_config::type_t::ERASURE_CODING,
                             .storages = m_num_instances,
                             .data_shards = m_num_instances - 1,
                             .parity_shards = 1,
                             .chunk_size_kib = 64 * KIBI_BYTE};
    global_data_view_config m_gdv_cfg;
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
};

BOOST_FIXTURE_TEST_SUITE(a_maintainer, basic_fixture)

BOOST_AUTO_TEST_CASE(is_created_and_destroys) {
    etcd_manager thread_local_etcd;
    temp_directory dir;
    service_config service_cfg{.working_dir = dir.path()};

    ec_maintainer maintainer(m_ioc, thread_local_etcd, m_group_cfg, 0,
                             service_cfg, m_gdv_cfg);
}

BOOST_AUTO_TEST_SUITE_END()

class fixture_for_ec_maintainer : public basic_fixture {
public:
    fixture_for_ec_maintainer()
        : basic_fixture{} {}

    void spawn_ec_maintainer(std::stop_token stoken, std::size_t storage_id) {
        etcd_manager thread_local_etcd;
        temp_directory dir;
        service_config service_cfg{.working_dir = dir.path()};

        auto maintainer = std::make_optional<ec_maintainer>(
            m_ioc, thread_local_etcd, m_group_cfg, storage_id, service_cfg,
            m_gdv_cfg);

        while (!stoken.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        maintainer.reset();
    }
};

class fixture_with_subscribers : public fixture_for_ec_maintainer {
public:
    fixture_with_subscribers()
        : fixture_for_ec_maintainer{} {

        for (std::size_t i = 0; i < m_num_instances; ++i) {
            threads[i] = std::jthread([&, i](std::stop_token stoken) {
                spawn_ec_maintainer(stoken, i);
            });
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ~fixture_with_subscribers() {
        for (auto& [key, thread] : threads) {
            if (!thread.get_stop_token().stop_requested()) {
                thread.request_stop();
            }
        }
    }

    bool wait_for_leader_key() {
        std::unique_lock<std::mutex> lock(cv_mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(5),
                         [&] { return leader_updated; })) {
            return false;
        }
        leader_updated = false;
        return true;
    }

    bool wait_for_group_state_key() {
        std::unique_lock<std::mutex> lock(cv_mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(10),
                         [&] { return group_state_updated; })) {
            return false;
        }
        group_state_updated = false;
        return true;
    }

    bool wait_for_storage_states_key() {
        std::unique_lock<std::mutex> lock(cv_mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(5),
                         [&] { return storage_states_updated; })) {
            return false;
        }
        storage_states_updated = false;
        return true;
    }

protected:
    std::condition_variable cv;
    std::mutex cv_mutex;
    bool leader_updated = false;
    bool group_state_updated = false;
    bool storage_states_updated = false;
    value_observer<int> m_leader_observer{
        ns::root.storage_groups[m_group_cfg.id].leader,
        candidate_observer::staging_id, [&](int leader_id) {
            std::lock_guard<std::mutex> lock(cv_mutex);
            leader_updated = true;
            cv.notify_one();
        }};
    value_observer<group_state> m_group_state_observer{
        ns::root.storage_groups[m_group_cfg.id].group_state,
        {},
        [&](group_state state) {
            std::lock_guard<std::mutex> lock(cv_mutex);
            group_state_updated = true;
            cv.notify_one();
        }};
    vector_observer<storage_state> m_storage_states_observer{
        ns::root.storage_groups[m_group_cfg.id].storage_states,
        m_num_instances,
        {},
        [&](std::size_t id, storage_state& state) {
            std::lock_guard<std::mutex> lock(cv_mutex);
            storage_states_updated = true;
            cv.notify_one();
        }};
    subscriber m_subscriber{
        "fixture",
        m_etcd,
        ns::root.storage_groups[m_group_cfg.id],
        {m_leader_observer, m_group_state_observer, m_storage_states_observer}};
    std::map<std::size_t, std::jthread> threads;
};

BOOST_FIXTURE_TEST_SUITE(multiple_ec_maintainers, fixture_with_subscribers)

BOOST_AUTO_TEST_CASE(find_who_is_leader) {
    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto leader_id = *m_leader_observer.get();
    BOOST_TEST(leader_id == candidate_observer::staging_id);

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    leader_id = *m_leader_observer.get();
    BOOST_TEST(leader_id != candidate_observer::staging_id);
    BOOST_TEST(leader_id < m_num_instances);
}

BOOST_AUTO_TEST_CASE(determine_healthy_group_state) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::HEALTHY);
}

BOOST_AUTO_TEST_CASE(assign_storages) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    auto states = m_storage_states_observer.get();
    for (auto state : states) {
        BOOST_TEST(*state == storage_state::ASSIGNED);
    }
}

BOOST_AUTO_TEST_CASE(handle_failover) {
    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto leader_id = *m_leader_observer.get();

    std::cout << "Kill the leader" << std::endl;
    threads[leader_id].request_stop();

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*m_leader_observer.get() == candidate_observer::staging_id);
    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*m_leader_observer.get() == candidate_observer::staging_id);
    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto new_leader_id = *m_leader_observer.get();
    BOOST_TEST(new_leader_id != candidate_observer::staging_id);
    BOOST_TEST(new_leader_id < m_num_instances);
    BOOST_TEST(new_leader_id != leader_id);
}

BOOST_AUTO_TEST_CASE(determine_degraded_group_state) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto leader_id = *m_leader_observer.get();

    auto cnt = 0ul;
    for (auto& [key, thread] : threads) {
        if ((candidate_observer::id_t)key == leader_id) {
            continue;
        }
        std::cout << std::format("Kill service {}", key) << std::endl;
        threads[key].request_stop();
        if (++cnt >= m_group_cfg.parity_shards)
            break;
    }

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::DEGRADED);
}

BOOST_AUTO_TEST_CASE(determine_degraded_group_state_when_leader_is_down) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto leader_id = *m_leader_observer.get();

    std::cout << std::format("Kill service {}", leader_id) << std::endl;
    threads[leader_id].request_stop();

    {
        if (wait_for_group_state_key() == false) {
            BOOST_FAIL("Callback was not called within the timeout period");
        }
        auto state = *m_group_state_observer.get();
        BOOST_TEST(state == group_state::UNDETERMINED);
    }

    {
        if (wait_for_group_state_key() == false) {
            BOOST_FAIL("Callback was not called within the timeout period");
        }
        auto state = *m_group_state_observer.get();
        BOOST_TEST(state == group_state::DEGRADED);
    }
}

BOOST_AUTO_TEST_CASE(determine_failed_group_state) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto leader_id = *m_leader_observer.get();

    auto cnt = 0ul;
    for (auto& [key, thread] : threads) {
        if ((candidate_observer::id_t)key == leader_id) {
            continue;
        }
        std::cout << std::format("Kill service {}", key) << std::endl;
        threads[key].request_stop();
        if (++cnt >= m_group_cfg.parity_shards)
            break;
    }

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::DEGRADED);

    threads[leader_id].request_stop();
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::UNDETERMINED);

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::FAILED);
}

BOOST_AUTO_TEST_CASE(determine_repairing_group_state) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto leader_id = *m_leader_observer.get();

    std::cout << std::format("Kill service {}", leader_id) << std::endl;
    threads[leader_id].request_stop();
    threads[leader_id].join();

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::UNDETERMINED);

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::DEGRADED);

    std::cout << std::format("Thread for storage {} created", leader_id)
              << std::endl;
    threads[leader_id] = std::jthread([&, leader_id](std::stop_token stoken) {
        spawn_ec_maintainer(stoken, leader_id);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::REPAIRING);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
