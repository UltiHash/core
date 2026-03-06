// Copyright 2026 UltiHash Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
