#define BOOST_TEST_MODULE "etcd manager tests"

#include "common/etcd/utils.h"

#include <boost/test/unit_test.hpp>
#include <memory>

using namespace std::chrono_literals;

namespace uh::cluster {

BOOST_AUTO_TEST_SUITE(when_client_has_system_failure_a_etcd_manager)

BOOST_AUTO_TEST_CASE(cannot_read_value_after_lease_time) {
    auto manager = std::make_unique<etcd_manager>(etcd_config{}, 2 /*second*/);
    const auto value = std::string("172.0.0.1");
    manager->put("/test/a", value);
    manager.reset();

    std::this_thread::sleep_for(2.5s);
    manager.reset(new etcd_manager({}, 2));
    auto read = manager->get("/test/a");

    BOOST_TEST(read == "");
    manager->clear_all();
}

BOOST_AUTO_TEST_CASE(
    cannot_read_value_always_because_lease_was_revoked_on_destroyer) {
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

    BOOST_TEST(read == "");
    manager->clear_all();
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
