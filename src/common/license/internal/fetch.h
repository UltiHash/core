#pragma once

// TODO: move file!
// use beast, or use existing implementation!

#include <boost/asio.hpp>
#include <common/types/common_types.h>
#include <random>

namespace uh::cluster::lic {

coro<std::string> fetch_response_body(boost::asio::io_context& io_context,
                                      const std::string& url,
                                      const std::string& username = "",
                                      const std::string& password = "");

/**
 * Exception handler for this fetch module.
 *
 * @return `true` means to "retry with exponential backoff"
 */
bool fetch_exception_handler(const std::exception& e);

template <typename T> class exponential_backoff {
public:
    using exception_handler = std::function<bool(const std::exception&)>;
    exponential_backoff(boost::asio::io_context& io_context, int max_retries,
                        int min_delay, int max_delay)
        : io_context_(io_context),
          max_retries_(max_retries),
          min_delay_(min_delay),
          max_delay_(max_delay) {}

    coro<T> run(
        std::function<coro<T>()> task,
        exception_handler exception_handler = [](const std::exception&) {
            return true;
        }) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min_delay_, max_delay_);

        int delay_ms = dis(gen);
        T rv;
        int retry_count = 0;

        while (retry_count < max_retries_) {
            bool success = false;
            try {
                rv = co_await task();
                success = true;
            } catch (const std::exception& e) {
                if (!exception_handler(e)) {
                    break;
                }
                retry_count++;
            }

            if (success) {
                break;
            }

            if (retry_count < max_retries_) {
                boost::asio::steady_timer timer(
                    io_context_, std::chrono::milliseconds(delay_ms));
                co_await timer.async_wait(boost::asio::use_awaitable);
                delay_ms *= 2;
            } else {
                std::cerr << "Max retries reached. Exiting." << std::endl;
                // TODO: throw exception, to remove etcd info from etcd
            }
        }
        co_return rv;
    }

private:
    boost::asio::io_context& io_context_;
    int max_retries_;
    int min_delay_;
    int max_delay_;
};

} // namespace uh::cluster::lic
