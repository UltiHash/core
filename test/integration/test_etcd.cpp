#define BOOST_TEST_MODULE "etcd tests"

#include "fakeit.hpp"
#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp>

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
    fixture()
        : etcd_address{"http://127.0.0.1:2379"},
          etcd_client{etcd_address} {
        When(Method(mock, handle_state_changes))
            .AlwaysDo([](const etcd::Response&) {});
    }

    ~fixture() { etcd_client.rmdir("/", true); }

protected:
    std::string etcd_address;
    etcd::Client etcd_client;
    etcd::Response response;
    Mock<callback_interface> mock;
};

BOOST_AUTO_TEST_SUITE(a_etcd_client)

BOOST_FIXTURE_TEST_CASE(gets_leasegrant, fixture) {
    auto resp = etcd_client.leasegrant(2).get();

    BOOST_TEST(resp.is_ok() == true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
