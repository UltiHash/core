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

    virtual ~basic_fixture() { m_etcd.clear_all(); }

protected:
    const std::size_t m_num_instances = 4;
    group_config m_group_cfg{.id = 0,
                             .type = group_config::type_t::ERASURE_CODING,
                             .storages = m_num_instances,
                             .data_shards = m_num_instances - 1,
                             .parity_shards = 1,
                             .stripe_size_kib = 64 * KIBI_BYTE};
    global_data_view_config m_gdv_cfg;
    etcd_manager m_etcd;
};

struct write_offset_interface {
    write_offset_interface(std::size_t val)
        : m_val{val} {}
    std::size_t get_write_offset() { return m_val; }
    void set_write_offset(std::size_t val) { m_val = val; }

private:
    std::size_t m_val;
};

BOOST_FIXTURE_TEST_SUITE(a_maintainer, basic_fixture)

BOOST_AUTO_TEST_CASE(is_created_and_destroys) {
    etcd_manager thread_local_etcd;
    temp_directory dir;
    service_config service_cfg{.working_dir = dir.path()};

    ec_maintainer maintainer(thread_local_etcd, m_group_cfg, 0, service_cfg,
                             m_gdv_cfg,
                             std::make_shared<write_offset_interface>(0));

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
}

BOOST_AUTO_TEST_SUITE_END()

class fixture_with_subscribers : public basic_fixture {
public:
    fixture_with_subscribers()
        : basic_fixture() {

        for (std::size_t i = 0; i < m_num_instances; ++i) {

            temp_directory dir;
            service_config service_cfg{.working_dir = dir.path()};

            m_etcds.push_back(std::make_unique<etcd_manager>());
            m_wo_interfaces.emplace_back(
                std::make_unique<write_offset_interface>(i * 1_KiB));

            m_ec_maintainers.emplace_back(
                std::make_unique<ec_maintainer<write_offset_interface>>(
                    *m_etcds.back(), m_group_cfg, i, service_cfg, m_gdv_cfg,
                    m_wo_interfaces.back()));
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
        candidate_observer::default_id, //
        [&](int leader_id) {
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
        "test_ec_maintainer",
        m_etcd,
        ns::root.storage_groups[m_group_cfg.id],
        {m_leader_observer, m_group_state_observer, m_storage_states_observer}};

    std::vector<std::unique_ptr<etcd_manager>> m_etcds;
    std::vector<std::shared_ptr<write_offset_interface>> m_wo_interfaces;
    std::vector<std::unique_ptr<ec_maintainer<write_offset_interface>>>
        m_ec_maintainers;
};

BOOST_FIXTURE_TEST_SUITE(multiple_ec_maintainers, fixture_with_subscribers)

#define TEST_FOR(CONDITION, TIMEOUT)                                           \
    do {                                                                       \
        auto start = std::chrono::steady_clock::now();                         \
        do {                                                                   \
            if ((CONDITION)) {                                                 \
                break;                                                         \
            }                                                                  \
            if ((std::chrono::steady_clock::now() - start) >= (TIMEOUT)) {     \
                BOOST_FAIL("Condition was not met within the timeout "         \
                           "period: " #CONDITION);                             \
            }                                                                  \
        } while (true);                                                        \
        BOOST_TEST_MESSAGE("waiting for \"" #CONDITION "\" passed");           \
    } while (false)

BOOST_AUTO_TEST_CASE(find_who_is_leader) {
    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
}

BOOST_AUTO_TEST_CASE(determine_their_maximum_offset_as_the_offset) {
    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
    auto leader_id = *m_leader_observer.get();
    LOG_DEBUG() << "Leader ID: " << leader_id;
    BOOST_TEST(leader_id != candidate_observer::default_id);
    BOOST_TEST(leader_id != candidate_observer::staging_id);
    BOOST_TEST(m_wo_interfaces[leader_id]->get_write_offset() ==
               (m_num_instances - 1) * 1_KiB);

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*m_group_state_observer.get() == group_state::HEALTHY);

    for (auto i = 0ul; i < m_num_instances; ++i) {
        m_wo_interfaces[i]->set_write_offset(1_MiB);
    }

    std::cout << "Kill the leader: " << leader_id << std::endl;
    m_ec_maintainers.at(leader_id).reset();

    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
    auto new_leader_id = *m_leader_observer.get();
    BOOST_TEST(m_wo_interfaces[new_leader_id]->get_write_offset() == 1_MiB);
}

BOOST_AUTO_TEST_CASE(determine_healthy_group_state) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*m_group_state_observer.get() == group_state::HEALTHY);
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
    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
    auto leader_id = *m_leader_observer.get();
    BOOST_TEST(leader_id != candidate_observer::default_id);
    BOOST_TEST(leader_id != candidate_observer::staging_id);

    std::cout << "Kill the leader: " << leader_id << std::endl;
    m_ec_maintainers.at(leader_id).reset();

    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
}

BOOST_AUTO_TEST_CASE(determine_degraded_group_state) {
    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
    auto leader_id = *m_leader_observer.get();
    BOOST_TEST(leader_id != candidate_observer::default_id);
    BOOST_TEST(leader_id != candidate_observer::staging_id);

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*m_group_state_observer.get() == group_state::HEALTHY);

    auto cnt = 0ul;
    for (auto i = 0ul; i < m_ec_maintainers.size(); ++i) {
        if (i == (std::size_t)leader_id)
            continue;
        std::cout << std::format("Kill service {}", i) << std::endl;
        m_ec_maintainers[i].reset();
        if (++cnt >= m_group_cfg.parity_shards)
            break;
    }

    TEST_FOR(wait_for_group_state_key() &&
                 *m_group_state_observer.get() == group_state::DEGRADED,
             4s);
}

BOOST_AUTO_TEST_CASE(determine_degraded_group_state_when_leader_is_down) {
    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
    auto leader_id = *m_leader_observer.get();
    BOOST_TEST(leader_id != candidate_observer::default_id);
    BOOST_TEST(leader_id != candidate_observer::staging_id);

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*m_group_state_observer.get() == group_state::HEALTHY);

    std::cout << std::format("Kill service {}", leader_id) << std::endl;
    m_ec_maintainers[leader_id].reset();

    TEST_FOR(wait_for_group_state_key() &&
                 *m_group_state_observer.get() == group_state::DEGRADED,
             4s);
}

BOOST_AUTO_TEST_CASE(determine_failed_group_state) {
    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
    auto leader_id = *m_leader_observer.get();
    BOOST_TEST(leader_id != candidate_observer::default_id);
    BOOST_TEST(leader_id != candidate_observer::staging_id);

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*m_group_state_observer.get() == group_state::HEALTHY);

    auto cnt = 0ul;
    for (auto i = 0ul; i < m_ec_maintainers.size(); ++i) {
        if (i == (std::size_t)leader_id)
            continue;
        std::cout << std::format("Kill service {}", i) << std::endl;
        m_ec_maintainers[i].reset();
        if (++cnt >= m_group_cfg.parity_shards)
            break;
    }

    TEST_FOR(wait_for_group_state_key() &&
                 *m_group_state_observer.get() == group_state::DEGRADED,
             4s);

    m_ec_maintainers[leader_id].reset();

    TEST_FOR(wait_for_group_state_key() &&
                 *m_group_state_observer.get() == group_state::FAILED,
             4s);
}

BOOST_AUTO_TEST_CASE(determine_repairing_group_state) {
    TEST_FOR(wait_for_leader_key() && *m_leader_observer.get() >= 0, 4s);
    auto leader_id = *m_leader_observer.get();
    BOOST_TEST(leader_id != candidate_observer::default_id);
    BOOST_TEST(leader_id != candidate_observer::staging_id);

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(*m_group_state_observer.get() == group_state::HEALTHY);

    std::cout << std::format("Kill service {}", leader_id) << std::endl;
    m_ec_maintainers[leader_id].reset();

    TEST_FOR(wait_for_group_state_key() &&
                 *m_group_state_observer.get() == group_state::DEGRADED,
             4s);

    {
        temp_directory dir;
        service_config service_cfg{.working_dir = dir.path()};
        m_ec_maintainers[leader_id] =
            std::make_unique<ec_maintainer<write_offset_interface>>(
                *m_etcds[leader_id], m_group_cfg, leader_id, service_cfg,
                m_gdv_cfg, m_wo_interfaces[leader_id]);
    }

    TEST_FOR(wait_for_group_state_key() &&
                 *m_group_state_observer.get() == group_state::REPAIRING,
             4s);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
