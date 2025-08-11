#define BOOST_TEST_MODULE "signals tests"

#include <boost/asio.hpp>

#define BOOST_PROCESS_VERSION 1

#include <boost/process/v1/args.hpp>
#include <boost/process/v1/child.hpp>
#include <boost/process/v1/env.hpp>
#include <boost/process/v1/environment.hpp>

#include <signal.h>

#include "test_config.h"

namespace bp = boost::process;

BOOST_AUTO_TEST_SUITE(a_signal_set)

BOOST_AUTO_TEST_CASE(supports_destroying_resource_on_async_handler) {
    std::promise<void> prom;
    auto fut = prom.get_future();
    auto executable = UH_BINARY_DIR "/test/unit/signals";
    std::cout << "executable: " << executable << std::endl;
    auto p = bp::child(executable);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    BOOST_TEST(kill(p.id(), SIGTERM) == 0);

    std::thread waiter([&] {
        p.wait();
        prom.set_value();
    });
    BOOST_TEST(
        (fut.wait_for(std::chrono::seconds(3)) != std::future_status::timeout));
    BOOST_TEST(p.exit_code() == 0);
    waiter.join();
}

BOOST_AUTO_TEST_SUITE_END()
