#define BOOST_TEST_MODULE "etcd manager tests"

#include "common/etcd/utils.h"
#include "common/telemetry/log.h"
#include "fakeit/fakeit.hpp"
#include "utils/system.h"

#include <boost/test/unit_test.hpp>

using namespace fakeit;
using namespace std::chrono_literals;

namespace uh::cluster {

class callback_interface {
public:
    virtual ~callback_interface() = default;
    virtual void handle_state_changes(const etcd::Response& response) = 0;
};

class fixture {
public:
    void setup() {
        auto log_config = log::config{
            .sinks = {log::sink_config{.type = log::sink_type::cout,
                                       .level = boost::log::trivial::debug,
                                       .service_role = DEDUPLICATOR_SERVICE}}};
        log::init(log_config);

        run_with_optional_sudo("systemctl start etcd");
        std::this_thread::sleep_for(1s);
    }

    fixture()
        : etcd{{}, lease_seconds} {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~fixture() {
        etcd.clear_all();
        std::this_thread::sleep_for(100ms);
    }

protected:
    static constexpr int lease_seconds = 30;
    etcd_config cfg;
    etcd_manager etcd;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(when_etcd_has_system_failure_a_etcd_manager)

BOOST_FIXTURE_TEST_CASE(
    recovers_previous_attached_watchers_to_watch_change_well_after_etcd_restarts,
    fixture) {
    etcd.watch("/test_1", [&cb = mock.get()](const etcd::Response& response) {
        cb.handle_state_changes(response);
    });
    BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl stop etcd"), 0);
    std::this_thread::sleep_for(1s);
    BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl start etcd"), 0);
    std::this_thread::sleep_for(1s);

    etcd.put("/test_1/a0", "172.0.0.1");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1);
}

BOOST_FIXTURE_TEST_CASE(reads_the_previously_written_value, fixture) {
    const auto value = std::string("172.0.0.1");
    etcd.put("/test_1/a0", value);

    BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl stop etcd"), 0);
    std::this_thread::sleep_for(1s);
    BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl start etcd"), 0);
    std::this_thread::sleep_for(1s);

    auto read = etcd.get("/test_1/a0");
    BOOST_TEST(value == read);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
