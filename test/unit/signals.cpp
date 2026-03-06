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

#include <boost/asio.hpp>
#include <iostream>
#include <thread>

int main() {
    boost::asio::io_context ioc;

    boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);

    std::cout << "[signals.cpp] waits... " << std::endl;

    signals.async_wait(
        [&](const boost::system::error_code& ec, int signal_number) {
            if (!ec) {
                std::cout << "[signals.cpp] Signal number " << signal_number
                          << std::endl;
                std::cout << "[signals.cpp] Gracefully stopping the "
                             "timer and exiting"
                          << std::endl;
            } else {
                std::cout << "[signals.cpp] Error " << ec.value() << " - "
                          << ec.message() << " - Signal number - "
                          << signal_number << std::endl;
            }
            ioc.stop();
        });

    auto workguard(boost::asio::make_work_guard(ioc));

    std::thread io_thread([&] { ioc.run(); });

    io_thread.join();
    return 0;
}
