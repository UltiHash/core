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
