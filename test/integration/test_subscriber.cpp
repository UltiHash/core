#define BOOST_TEST_MODULE "subscriber tests"

#include <boost/test/unit_test.hpp>

#include "test_config.h"

#include <common/etcd/namespace.h>
#include <common/etcd/subscriber.h>

namespace uh::cluster::storage {

class fixture {
public:
    fixture() {}

    ~fixture() {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
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
    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;

    std::condition_variable cv;
    std::mutex cv_mutex;
    bool leader_updated = false;
    std::size_t m_group_id = 0;
    value_observer<int> m_leader_observer{
        ns::root.storage_groups[m_group_id].leader, -1, [&](int new_leader) {
            std::lock_guard<std::mutex> lock(cv_mutex);
            leader_updated = true;
            cv.notify_one(); // Notify the waiting thread
            std::cerr << "Leader updated: " << new_leader << std::endl;
        }};
    subscriber m_subscriber{"fixture",
                            m_etcd,
                            ns::root.storage_groups[m_group_id].leader,
                            {m_leader_observer}};
};

BOOST_FIXTURE_TEST_SUITE(a_value_observer, fixture)

BOOST_AUTO_TEST_CASE(returns_default_value_when_the_key_has_no_value) {
    BOOST_TEST(*m_leader_observer.get() == -1);
}

BOOST_AUTO_TEST_CASE(returns_default_value_when_the_key_has_empty_value) {

    m_etcd.put(
        ns::root.storage_groups[m_group_id].leader,
        serialize<candidate_observer::id_t>(candidate_observer::staging_id));

    if (wait_for_leader_key() == false) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }

    BOOST_TEST(*m_leader_observer.get() == candidate_observer::staging_id);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
