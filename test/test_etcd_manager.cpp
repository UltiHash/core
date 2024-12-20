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
        : manager{} {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~fixture() {
        manager.clear_all();
        std::this_thread::sleep_for(1s);
    }

protected:
    etcd_config cfg;
    etcd_manager manager;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(a_etcd_manager)

BOOST_FIXTURE_TEST_CASE(cannot_call_cb_without_putting_something, fixture) {
    manager.watch("/test_1",
                  [&cb = mock.get()](const etcd::Response& response) {
                      cb.handle_state_changes(response);
                  });

    (void)(0);
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(0);
}

BOOST_FIXTURE_TEST_CASE(checks_change_when_calling_put_after_watch_created,
                        fixture) {
    manager.watch("/test_1",
                  [&cb = mock.get()](const etcd::Response& response) {
                      cb.handle_state_changes(response);
                  });

    manager.put("/test_1/a0", "172.0.0.1");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1_Time);
}

BOOST_FIXTURE_TEST_CASE(checks_change_also_when_etcd_restarts, fixture) {
    manager.put("/test_0/a0", "172.0.0.1");
    std::this_thread::sleep_for(100ms);
    manager.watch("/test_1",
                  [&cb = mock.get()](const etcd::Response& response) {
                      cb.handle_state_changes(response);
                  });
    BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl stop etcd"), 0);
    std::this_thread::sleep_for(1s);
    BOOST_CHECK_EQUAL(run_with_optional_sudo("systemctl start etcd"), 0);
    std::this_thread::sleep_for(1s);

    manager.put("/test_1/a0", "172.0.0.1");
    std::this_thread::sleep_for(100ms);

    Verify(Method(mock, handle_state_changes)).Exactly(1);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
