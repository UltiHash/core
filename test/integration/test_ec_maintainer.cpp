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

class fixture {
public:
    fixture()
        : m_etcd{} {}

    virtual ~fixture() {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    const std::size_t m_num_instances = 3;
    group_config m_group_cfg{.id = 1,
                             .type = group_config::type_t::ERASURE_CODING,
                             .storages = m_num_instances,
                             .data_shards = m_num_instances - 1,
                             .parity_shards = 1,
                             .chunk_size_kib = 64 * KIBI_BYTE};
    global_data_view_config m_gdv_cfg;
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
};

BOOST_FIXTURE_TEST_SUITE(a_state_manager, fixture)

BOOST_AUTO_TEST_CASE(is_created_and_destroys) {
    const auto storage_id = 0ul;
    etcd_manager local_etcd;
    temp_directory dir;
    service_config service_cfg{.working_dir = dir.path()};
    auto reg = storage_registry(local_etcd, m_group_cfg.id, storage_id,
                                service_cfg.working_dir);

    state_manager sm(m_ioc, local_etcd, m_group_cfg, storage_id, m_gdv_cfg);
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(a_parcitipant, fixture)

BOOST_AUTO_TEST_CASE(is_created_and_destroys) {
    const auto storage_id = 0ul;
    etcd_manager local_etcd;
    temp_directory dir;
    service_config service_cfg{.working_dir = dir.path()};
    auto reg = storage_registry(local_etcd, m_group_cfg.id, storage_id,
                                service_cfg.working_dir);

    leader l(m_ioc, local_etcd, m_group_cfg, storage_id, m_gdv_cfg, reg);
    follower f(local_etcd, m_group_cfg, storage_id, reg);
}

BOOST_AUTO_TEST_SUITE_END()

class fixture_for_ec_maintainer : public fixture {
public:
    fixture_for_ec_maintainer()
        : fixture{} {}

    void spawn_ec_maintainer(std::stop_token stoken, std::size_t storage_id) {
        etcd_manager thread_local_etcd;
        temp_directory dir;
        service_config service_cfg{.working_dir = dir.path()};

        ec_maintainer maintainer(m_ioc, thread_local_etcd, m_group_cfg,
                                 storage_id, service_cfg, m_gdv_cfg);

        while (!stoken.stop_requested()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        std::cout << "Thread finished or was canceled" << std::endl;
    }

    bool wait_for_leader_key() {
        std::unique_lock<std::mutex> lock(cv_mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(2),
                         [&] { return leader_updated; })) {
            return false;
        }
        leader_updated = false;
        return true;
    }

protected:
    std::condition_variable cv;
    std::mutex cv_mutex;
    bool leader_updated = false;
    value_observer<int> m_leader_observer{
        ns::root.storage_groups[m_group_cfg.id].leader, candidate::staging_id,
        [&](int new_leader) {
            std::lock_guard<std::mutex> lock(cv_mutex);
            leader_updated = true;
            cv.notify_one(); // Notify the waiting thread
            std::cerr << "Leader updated: " << new_leader << std::endl;
        }};
    subscriber m_subscriber{m_etcd,
                            ns::root.storage_groups[m_group_cfg.id].leader,
                            {m_leader_observer}};
};

BOOST_FIXTURE_TEST_SUITE(a_ec_maintainer, fixture_for_ec_maintainer)

BOOST_AUTO_TEST_CASE(is_created_and_destroys) {
    etcd_manager thread_local_etcd;
    temp_directory dir;
    service_config service_cfg{.working_dir = dir.path()};

    ec_maintainer maintainer(m_ioc, thread_local_etcd, m_group_cfg, 0,
                             service_cfg, m_gdv_cfg);
}

BOOST_AUTO_TEST_CASE(supports_election) {
    std::vector<std::jthread> threads;

    for (std::size_t i = 0; i < m_num_instances; ++i) {
        threads.emplace_back(
            [&, i](std::stop_token stoken) { spawn_ec_maintainer(stoken, i); });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    auto leader_id = *m_leader_observer.get();
    std::cout << "Initial leader: " << leader_id << std::endl;
    BOOST_TEST(leader_id != candidate::staging_id);
    BOOST_TEST(leader_id < m_num_instances);

    for (auto& thread : threads) {
        if (!thread.get_stop_token().stop_requested()) {
            thread.request_stop();
            std::cout << "Stop requested for thread " << &thread << std::endl;
        }
    }
}

BOOST_AUTO_TEST_CASE(handles_failover) {
    std::vector<std::jthread> threads;
    for (std::size_t i = 0; i < m_num_instances; ++i) {
        threads.emplace_back(
            [&, i](std::stop_token stoken) { spawn_ec_maintainer(stoken, i); });
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    auto leader_id = *m_leader_observer.get();
    std::cout << "Initial leader: " << leader_id << std::endl;

    // Interrupt the leader's thread
    std::cout << "Kill the leader" << std::endl;
    threads[leader_id].request_stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(*m_leader_observer.get() == candidate::staging_id);
    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    auto new_leader_id = *m_leader_observer.get();
    std::cout << "New leader: " << new_leader_id << std::endl;
    BOOST_TEST(new_leader_id != candidate::staging_id);
    BOOST_TEST(new_leader_id < m_num_instances);
    BOOST_TEST(new_leader_id != leader_id);

    for (auto& thread : threads) {
        if (!thread.get_stop_token().stop_requested()) {
            thread.request_stop();
            std::cout << "Stop requested for thread " << &thread << std::endl;
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
