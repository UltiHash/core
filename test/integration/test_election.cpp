#define BOOST_TEST_MODULE "election tests"

#include <common/election/candidate.h>
#include <common/election/observer.h>

#include <boost/test/unit_test.hpp>

#include <future>

using namespace std::chrono_literals;

namespace uh::cluster {

BOOST_AUTO_TEST_SUITE(a_observer)

BOOST_AUTO_TEST_CASE(is_created_and_destroyed_well) {
    auto etcd_observer = etcd_manager();
    auto o = observer(etcd_observer, "group_a");
    BOOST_TEST(o.get() == "");
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(a_candidate)

BOOST_AUTO_TEST_CASE(gets_leadership_when_no_other_candidates) {
    auto etcd_observer = etcd_manager();
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto o = observer(etcd_observer, "group_a", [&]() { p.set_value(); });

    auto etcd_candidate_1 = etcd_manager();
    auto c_1 = std::make_optional<candidate>(etcd_candidate_1, "group_a", "1");
    if (f.wait_for(5s) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(o.get() == "1");
    c_1.reset(); // candidate resigns
}

BOOST_AUTO_TEST_CASE(gets_leadership_when_it_is_created_first) {
    auto etcd_observer = etcd_manager();
    std::promise<void> p;
    std::future<void> f = p.get_future();
    auto o = observer(etcd_observer, "group_a", [&]() { p.set_value(); });

    auto etcd_candidate_1 = etcd_manager();
    auto c_1 = std::make_optional<candidate>(etcd_candidate_1, "group_a", "1");
    // etcd_manager etcd_candidate_2;
    // auto c_2 = candidate(etcd_candidate_2, "group_a", "2");
    if (f.wait_for(5s) == std::future_status::timeout) {
        BOOST_FAIL("Callback was not called within the timeout period");
    }
    BOOST_TEST(o.get() == "1");
    c_1.reset(); // candidate resigns
}

BOOST_AUTO_TEST_CASE(gets_leadership_when_previous_leader_goes_down) {
    etcd_manager etcd_observer;
    auto o = observer(etcd_observer, "group_a");
    etcd_manager etcd_candidate_1;
    auto c_1 = std::make_optional<candidate>(etcd_candidate_1, "group_a", "1");

    etcd_manager etcd_candidate_2;
    auto c_2 = candidate(etcd_candidate_2, "group_a", "2");
    c_1.reset();

    std::this_thread::sleep_for(2s);
    BOOST_TEST(o.get() == "2");
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
