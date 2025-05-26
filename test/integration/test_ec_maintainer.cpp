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
        std::this_thread::sleep_for(500ms);
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
    boost::asio::io_context m_ioc;
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

    ec_maintainer maintainer(m_ioc, thread_local_etcd, m_group_cfg, 0,
                             service_cfg, m_gdv_cfg,
                             std::make_shared<write_offset_interface>(0));
    auto thread = std::thread([&] {
        try {
            m_ioc.run();
        } catch (...) {
        }
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    m_ioc.stop();
    thread.join();
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
                    m_ioc, *m_etcds.back(), m_group_cfg, i, service_cfg,
                    m_gdv_cfg, m_wo_interfaces.back()));
        }

        m_thread = std::make_optional<std::thread>([&] {
            try {
                m_ioc.run();
            } catch (...) {
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    ~fixture_with_subscribers() {
        m_ioc.stop();
        m_thread->join();
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

    std::vector<std::shared_ptr<write_offset_interface>> m_wo_interfaces;

protected:
    std::condition_variable cv;
    std::mutex cv_mutex;
    bool leader_updated = false;
    bool group_state_updated = false;
    bool storage_states_updated = false;
    value_observer<int> m_leader_observer{
        ns::root.storage_groups[m_group_cfg.id].leader,
        candidate_observer::default_id, [&](int leader_id) {
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
    std::vector<std::unique_ptr<ec_maintainer<write_offset_interface>>>
        m_ec_maintainers;
    std::optional<std::thread> m_thread;
};

BOOST_FIXTURE_TEST_SUITE(multiple_ec_maintainers, fixture_with_subscribers)

BOOST_AUTO_TEST_CASE(find_who_is_leader) {
    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    BOOST_TEST(*m_leader_observer.get() != candidate_observer::default_id);
}

BOOST_AUTO_TEST_CASE(determine_their_maximum_offset_as_the_offset) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto leader_id = *m_leader_observer.get();
    BOOST_TEST(m_wo_interfaces[leader_id]->get_write_offset() ==
               (m_num_instances - 1) * 1_KiB);
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
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    BOOST_TEST(*m_leader_observer.get() != candidate_observer::default_id);

    auto leader_id = *m_leader_observer.get();

    std::cout << "Kill the leader: " << leader_id << std::endl;
    m_ec_maintainers.at(leader_id).reset();

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    // deletion
    BOOST_TEST(*m_leader_observer.get() == candidate_observer::default_id);

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    // creation of staging_id
    BOOST_TEST(*m_leader_observer.get() == candidate_observer::staging_id);

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    // proclamation
    auto new_leader_id = *m_leader_observer.get();
    BOOST_TEST(new_leader_id >= 0);
    BOOST_TEST(new_leader_id < m_num_instances);
    BOOST_TEST(new_leader_id != leader_id);
}

BOOST_AUTO_TEST_CASE(determine_degraded_group_state) {
    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto leader_id = *m_leader_observer.get();
    BOOST_TEST(leader_id != candidate_observer::default_id);

    auto cnt = 0ul;
    for (auto i = 0ul; i < m_ec_maintainers.size(); ++i) {
        if (i == (std::size_t)leader_id)
            continue;
        std::cout << std::format("Kill service {}", i) << std::endl;
        m_ec_maintainers[i].reset();
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
    m_ec_maintainers[leader_id].reset();

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
    for (auto i = 0ul; i < m_ec_maintainers.size(); ++i) {
        if (i == (std::size_t)leader_id)
            continue;
        std::cout << std::format("Kill service {}", i) << std::endl;
        m_ec_maintainers[i].reset();
        if (++cnt >= m_group_cfg.parity_shards)
            break;
    }

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::DEGRADED);

    m_ec_maintainers[leader_id].reset();

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
    m_ec_maintainers[leader_id].reset();

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    auto state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::DEGRADED);

    std::cout << std::format("Thread for storage {} created", leader_id)
              << std::endl;

    {
        temp_directory dir;
        service_config service_cfg{.working_dir = dir.path()};
        m_ec_maintainers[leader_id] =
            std::make_unique<ec_maintainer<write_offset_interface>>(
                m_ioc, *m_etcds[leader_id], m_group_cfg, leader_id, service_cfg,
                m_gdv_cfg, m_wo_interfaces[leader_id]);
    }

    if (wait_for_group_state_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    state = *m_group_state_observer.get();
    BOOST_TEST(state == group_state::REPAIRING);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
