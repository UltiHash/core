#pragma once

#include <common/types/common_types.h>
#include <random>

namespace uh::cluster::lic {

enum class backoff_action : uint8_t { RETRY, ABORT };

template <typename T> class exponential_backoff {
public:
    using exception_handler =
        std::function<backoff_action(const std::exception&)>;
    exponential_backoff(boost::asio::io_context& io_context, int max_retries,
                        int min_delay, int max_delay)
        : io_context_(io_context),
          max_retries_(max_retries),
          min_delay_(min_delay),
          max_delay_(max_delay) {}

    coro<T> run(std::function<coro<T>()> task,
                exception_handler exception_handler) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min_delay_, max_delay_);

        int delay_ms = dis(gen);

        int retry_count = 0;
        while (retry_count++ < max_retries_) {
            try {
                auto rv = co_await task();
                co_return rv;
            } catch (const std::exception& e) {
                if (exception_handler(e) == backoff_action::ABORT) {
                    throw;
                }
            }

            boost::asio::steady_timer timer(
                io_context_, std::chrono::milliseconds(delay_ms));
            co_await timer.async_wait(boost::asio::use_awaitable);
            delay_ms *= 2;
        }

        throw std::runtime_error("Max retries reached. Exiting.");
    }

private:
    boost::asio::io_context& io_context_;
    int max_retries_;
    int min_delay_;
    int max_delay_;
};

} // namespace uh::cluster::lic
