#include "payg_updater.h"

#include "fetch.h"
#include "internal/payg.h"

#include "common/etcd/namespace.h"

namespace uh::cluster::lic {

coro<void> payg_updater::update() {
    // TODO: do DI for exp_backoff and fetch_exception_handler
    auto backoff = exponential_backoff<std::string>{m_ioc, 7, 100, 200};
    try {
        auto str = co_await backoff.run(
            [&]() -> coro<std::string> { co_return co_await m_get_license(); },
            m_exception_handler);

        auto handler = payg_handler(str);

        m_etcd.put(etcd_payg_license, handler.to_string());
    } catch (const std::runtime_error& e) {
        std::cout << "License check failed: " << e.what() << std::endl;
    } catch (...) {
    }
    co_return;
}
coro<void> payg_updater::periodic_update(std::chrono::seconds interval) {
    while (true) {
        auto start_time = std::chrono::steady_clock::now();

        co_await update();

        auto end_time = std::chrono::steady_clock::now();
        auto elapsed_time = end_time - start_time;
        auto sleep_duration = interval - elapsed_time;

        if (sleep_duration > 0s) {
            boost::asio::steady_timer timer(m_ioc, sleep_duration);
            co_await timer.async_wait(boost::asio::use_awaitable);
        }
    }
}

} // namespace uh::cluster::lic

namespace uh::cluster {

coro<void> periodic_executor(boost::asio::io_context& io_context,
                             std::chrono::seconds interval,
                             std::function<coro<void>()> task) {
    while (true) {
        auto start_time = std::chrono::steady_clock::now();

        co_await task();

        auto end_time = std::chrono::steady_clock::now();
        auto elapsed_time = end_time - start_time;
        auto sleep_duration = interval - elapsed_time;

        if (sleep_duration > 0s) {
            boost::asio::steady_timer timer(io_context, sleep_duration);
            co_await timer.async_wait(boost::asio::use_awaitable);
        }
    }
}

} // namespace uh::cluster
