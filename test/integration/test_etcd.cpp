#define BOOST_TEST_MODULE "etcd tests"

#include <etcd/Client.hpp>
#include <etcd/KeepAlive.hpp>
#include <etcd/Watcher.hpp>

#include <boost/test/unit_test.hpp>

using namespace fakeit;
using namespace std::chrono_literals;

namespace uh::cluster {

class fixture {
public:
    fixture()
        : etcd_address{"http://127.0.0.1:2379"},
          etcd_client{etcd_address} {}

    ~fixture() { etcd_client.rmdir("/", true); }

protected:
    std::string etcd_address;
    etcd::Client etcd_client;
    etcd::Response response;
};

BOOST_AUTO_TEST_SUITE(a_etcd_client)

BOOST_FIXTURE_TEST_CASE(watches_changes_on_the_given_key, fixture) {
    etcd_client.put("test0", "initial_value");
    std::promise<void> promise;
    std::future<void> future = promise.get_future();
    std::shared_ptr<etcd::Watcher> watcher;
    watcher.reset(new etcd::Watcher(
        etcd_client, "test0",
        [&](const etcd::Response& response) { promise.set_value(); },
        true /*recursive*/));

    etcd_client.put("test0", "updated_value");

    BOOST_CHECK(future.wait_for(2s) != std::future_status::timeout);
}

BOOST_FIXTURE_TEST_CASE(reads_written_value, fixture) {
    etcd_client.put("/foo/bar", "1");

    auto resp = etcd_client.get("/foo/bar");

    BOOST_TEST(resp.is_ok() == true);
    BOOST_TEST(resp.value().as_string() == "1");
}

BOOST_FIXTURE_TEST_CASE(gets_leasegrant, fixture) {
    auto resp = etcd_client.leasegrant(2).get();

    BOOST_TEST(resp.is_ok() == true);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace uh::cluster
