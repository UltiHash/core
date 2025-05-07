#define BOOST_TEST_MODULE "election tests"

#include <common/etcd/candidate.h>
#include <common/etcd/utils.h>
#include <storage/group/externals.h>

#include <boost/test/unit_test.hpp>

#include <future>

using namespace std::chrono_literals;

namespace uh::cluster::storage {

class fixture {
public:
    fixture()
        : m_etcd{} {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
        subscriber.emplace(m_etcd, group_id, num_storages,
                           service_factory<storage_interface>(m_ioc, 2), [&]() {
                               std::lock_guard<std::mutex> lock(cv_mutex);
                               callback_called = true;
                               cv.notify_one();
                           });
    }

    ~fixture() {
        subscriber.reset();
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    void wait_for_callback() {
        std::unique_lock<std::mutex> lock(cv_mutex);
        if (!cv.wait_for(lock, std::chrono::seconds(5),
                         [&] { return callback_called; })) {
            BOOST_FAIL("Callback was not called within the timeout period");
        }
        callback_called = false;
    }

    const std::size_t group_id = 11ul, num_storages = 7ul, storage_id = 2ul;

    std::condition_variable cv;
    std::mutex cv_mutex;
    bool callback_called = false;

    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    std::optional<externals_subscriber> subscriber;
};

BOOST_FIXTURE_TEST_SUITE(a_candidate, fixture)

BOOST_AUTO_TEST_CASE(gets_leadership_when_no_other_candidates) {
    auto c1 =
        candidate(m_etcd, ns::root.storage_groups[group_id].leader, storage_id);

    wait_for_callback();
    BOOST_TEST(*subscriber->get_leader() == storage_id);
}

BOOST_AUTO_TEST_CASE(loose_leadership_when_it_goes_down) {
    auto c1 = std::make_optional<candidate>(
        m_etcd, ns::root.storage_groups[group_id].leader, storage_id);

    wait_for_callback(); // proclamation

    c1.reset();

    wait_for_callback(); // resignation

    BOOST_TEST(*subscriber->get_leader() == -1);
}

BOOST_AUTO_TEST_CASE(gets_leadership_when_previous_leader_goes_down) {

    auto c1 = std::make_optional<candidate>(
        m_etcd, ns::root.storage_groups[group_id].leader, 1);

    wait_for_callback(); // proclamation

    etcd_manager etcd2;
    auto c2 =
        candidate(etcd2, ns::root.storage_groups[group_id].leader, storage_id);

    c1.reset();

    wait_for_callback(); // resignation
    wait_for_callback(); // preclamation
    wait_for_callback(); // proclamation

    BOOST_TEST(*subscriber->get_leader() == storage_id);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
