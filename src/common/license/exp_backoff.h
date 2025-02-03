#pragma once

#include <common/network/http_client.h>
#include <common/telemetry/log.h>
#include <common/types/common_types.h>
#include <random>

namespace uh::cluster {

enum class backoff_action : uint8_t { RETRY, ABORT };

template <typename T> class exponential_backoff {
public:
    using exception_handler =
        std::function<backoff_action(const std::exception&)>;
    exponential_backoff(boost::asio::io_context& io_context, int max_retries,
                        int min_delay_ms, int max_delay_ms)
        : m_ioc(io_context),
          m_max_retries(max_retries),
          m_min_delay_ms(min_delay_ms),
          m_max_delay_ms(max_delay_ms) {}

    coro<T> run(std::function<coro<T>()> task) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(m_min_delay_ms, m_max_delay_ms);

        int delay_ms = dis(gen);

        int retry_count = 0;
        while (retry_count++ < m_max_retries) {
            try {
                auto rv = co_await task();
                co_return rv;
            } catch (const std::system_error& se) {

                auto status_code = se.code().value();
                const auto& category = dynamic_cast<const http_error_category&>(
                    se.code().category());
                auto message = category.message(status_code);

                LOG_DEBUG() << "Error code: " << status_code;
                LOG_DEBUG() << "Error message: " << message;

                if (action(se.code().value()) == backoff_action::ABORT)
                    throw;
            } catch (...) {
                throw;
            }

            boost::asio::steady_timer timer(
                m_ioc, std::chrono::milliseconds(delay_ms));
            co_await timer.async_wait(boost::asio::use_awaitable);
            delay_ms *= 2;
        }

        throw std::runtime_error("Max retries reached. Exiting.");
    }

private:
    boost::asio::io_context& m_ioc;
    int m_max_retries;
    int m_min_delay_ms;
    int m_max_delay_ms;

    static backoff_action action(int ev) {
        if (ev == 429)
            return backoff_action::RETRY;

        if (500 <= ev && ev < 600)
            return backoff_action::RETRY;

        return backoff_action::ABORT;
    }
};

} // namespace uh::cluster
