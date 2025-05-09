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
        : m_etcd{} {}

    ~fixture() {
        m_etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    void setup_promises_and_subscriber(size_t expected_callbacks,
                                       size_t group_id, size_t num_storages) {
        promises.resize(expected_callbacks);
        futures.clear();
        for (auto& p : promises) {
            futures.push_back(p.get_future());
        }
        callback_count = 0;
        subscriber.emplace(m_etcd, group_id, num_storages,
                           service_factory<storage_interface>(m_ioc, 2),
                           [&](etcd_manager::response) {
                               if (callback_count < promises.size()) {
                                   promises[callback_count].set_value();
                               }
                               ++callback_count;
                           });
    }

    void wait_for_callbacks() {
        for (size_t i = 0; i < promises.size(); ++i) {
            if (futures[i].wait_for(std::chrono::seconds(5)) ==
                std::future_status::timeout) {
                BOOST_FAIL("Callback was not called within the timeout period");
            }
        }
    }

    boost::asio::io_context m_ioc;
    etcd_manager m_etcd;
    std::vector<std::promise<void>> promises;
    std::vector<std::future<void>> futures;
    size_t callback_count = 0;
    std::optional<externals_subscriber> subscriber;
};

BOOST_FIXTURE_TEST_SUITE(a_candidate, fixture)

BOOST_AUTO_TEST_CASE(gets_leadership_when_no_other_candidates) {
    const auto group_id = 11ul, num_storages = 7ul, storage_id = 3ul;

    // 2 promises, for preemption and proclamation
    setup_promises_and_subscriber(2, group_id, num_storages);

    auto c1 =
        candidate(m_etcd, ns::root.storage_groups[group_id].leader, storage_id);

    wait_for_callbacks();

    BOOST_TEST(*subscriber->get_leader() == storage_id);
}

BOOST_AUTO_TEST_CASE(loose_leadership_when_it_goes_down) {
    const auto group_id = 11ul, num_storages = 7ul, storage_id = 2ul;

    // For preemption, proclamation and resignation
    setup_promises_and_subscriber(3, group_id, num_storages);

    auto c1 = std::make_optional<candidate>(
        m_etcd, ns::root.storage_groups[group_id].leader, storage_id);

    c1.reset();

    wait_for_callbacks();

    BOOST_TEST(*subscriber->get_leader() == -1);
}

BOOST_AUTO_TEST_CASE(gets_leadership_when_previous_leader_goes_down) {
    const auto group_id = 11ul, num_storages = 7ul, storage_id = 2ul;

    // For preemption, proclamation, resignation for the first candidate,
    // and another preemption-proclamation for the second candidate
    setup_promises_and_subscriber(5, group_id, num_storages);

    auto c1 = std::make_optional<candidate>(
        m_etcd, ns::root.storage_groups[group_id].leader, 1);

    auto c2 =
        candidate(m_etcd, ns::root.storage_groups[group_id].leader, storage_id);

    c1.reset();

    wait_for_callbacks();

    BOOST_TEST(*subscriber->get_leader() == storage_id);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster::storage
