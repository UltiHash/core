#include "publisher.h"

#include "internal/fetch.h"

#include "payg.h"

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

coro<void> license_handler(boost::asio::io_context& io_context,
                           etcd_manager& etcd) {
    // TODO: replace url with https://<url>/v1/license
    // UH_CUSTOMER_ID and UH_ACCESS_TOKEN.
    const std::string url{"example.com"};
    const std::string username{""};
    const std::string password{""};
    const std::string dummy_license_string{R"({
    "customer_id": "big corp xy",
    "license_type": "freemium",
    "storage_cap": 10240,
    "ec": {
        "enabled": true,
        "max_group_size": 10
    },
    "replication": {
        "enabled": true,
        "max_replicas": 3
    },
    "signature": "Bplb7lZQIK+mIXyPZKRNRIjara5EqxrCz8M5FDlPfqDrlMppL43axS7Ccd9TyuL4v03zHsFzPOyW7k+L+uouBw=="
})"};

    try {
        auto str = co_await exponential_backoff<std::string>(
            io_context, [&]() -> coro<std::string> {
                co_return co_await fetch_response_body(io_context, url,
                                                       username, password);
            });

        // LOG_INFO() << "str: " << str;
        // TODO: remove below
        str = dummy_license_string;

        // TODO: do not skip checking
        auto license = check_payg_license(str);

        broadcast(etcd, license);
    } catch (const std::runtime_error& e) {
        std::cout << "License check failed: " << e.what() << std::endl;
    } catch (...) {
    }
}

} // namespace uh::cluster
