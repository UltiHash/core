#define BOOST_TEST_MODULE "etcd manager tests"

#include "common/etcd/utils.h"
#include "common/telemetry/log.h"
#include "fakeit/fakeit.hpp"
#include "utils/system.h"

#include <boost/test/unit_test.hpp>
#include <memory>

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

        std::this_thread::sleep_for(1s);
    }

    fixture() {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~fixture() { std::this_thread::sleep_for(1s); }

protected:
    etcd_config cfg;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(when_client_has_system_failure_a_etcd_manager)

BOOST_FIXTURE_TEST_CASE(can_read_value_before_lease_time, fixture) {
    auto manager =
        std::make_unique<etcd_manager>(etcd_config{}, 10 /*seconds*/);
    const auto value = std::string("172.0.0.1");
    manager->put("/test/a", value);
    std::this_thread::sleep_for(1s);
    manager.reset();
    std::this_thread::sleep_for(1s);

    manager.reset(new etcd_manager({}, 10));
    std::this_thread::sleep_for(1s);
    auto read = manager->get("/test/a");

    BOOST_TEST(read == value);
    manager->clear_all();
}

BOOST_FIXTURE_TEST_CASE(cannot_read_value_after_lease_time, fixture) {
    auto manager = std::make_unique<etcd_manager>(etcd_config{}, 2 /*second*/);
    const auto value = std::string("172.0.0.1");
    manager->put("/test/a", value);
    manager.reset();
    // std::this_thread::sleep_for(1.2s);
    //
    std::this_thread::sleep_for(2.5s);
    manager.reset(new etcd_manager({}, 2));
    auto read = manager->get("/test/a");
    //
    BOOST_TEST(read == "");
    manager->clear_all();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
