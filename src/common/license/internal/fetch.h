#pragma once

#include <boost/asio.hpp>
#include <common/types/common_types.h>
#include <random>

namespace uh::cluster {

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& url,
                                      const std::string& username = "",
                                      const std::string& password = "");

template <typename T>
coro<T> exponential_backoff(boost::asio::io_context& io_context,
                            std::function<coro<T>()> task) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(100, 200);

    int delay_ms = dis(gen);
    T rv;
    // TODO: If we try 10 times, total retry time is 13.6 hours.
    // In my opinion, trying 7 times is enough. In this case, total 0.85 hours.
    int max_retries = 7;
    int retry_count = 0;

    while (retry_count < max_retries) {
        bool success = false;
        try {
            // TODO: What should we do when the license server is down
            // for one hour? Should I clear license information on etcd, right?
            rv = co_await task();
            success = true;

        } catch (const std::system_error& e) {
            std::cout << "Caught system_error: " << e.what() << std::endl;
            std::cout << "Error code: " << e.code() << std::endl;
            std::cout << "Error message: " << e.code().message() << std::endl;
            // TODO: Implement exponential backoff.
            retry_count++;
        } catch (const std::runtime_error& e) {
            std::cout << "Caught runtime_error: " << e.what() << std::endl;
        }

        if (success) {
            break;
        }

        if (retry_count < max_retries) {
            boost::asio::steady_timer timer(
                io_context, std::chrono::milliseconds(delay_ms));
            co_await timer.async_wait(boost::asio::use_awaitable);
            delay_ms *= 2;
        } else {
            std::cerr << "Max retries reached. Exiting." << std::endl;
            // TODO: throw exception, to remove etcd info from etcd
        }
    }
    co_return rv;
}

} // namespace uh::cluster
