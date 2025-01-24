#pragma once

#include "common/etcd/ec_groups/ec_get_handler.h"
#include "config.h"
#include "fetch.h"

#include <common/etcd/ec_groups/ec_group_maintainer.h>
#include <common/etcd/service_discovery/service_maintainer.h>
#include <common/telemetry/log.h>
#include <common/utils/io_context_runner.h>
#include <config/configuration.h>

#include <boost/asio/spawn.hpp>
#include <boost/asio/steady_timer.hpp>

#include <common/license/payg.h>
#include <common/license/test.h>
#include <common/utils/strings.h>

namespace uh::cluster::coordinator {

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
        auto str = co_await exponential_backoff(
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

class service {
public:
    service(const service_config& service, const coordinator_config& cc)
        : m_etcd{service.etcd_config},
          m_ioc(cc.thread_count),

          m_ioc_runner(m_ioc, cc.thread_count),
          m_ec_maintainer(m_ioc, 1, 0, m_etcd, true),

          m_storage_maintainer(
              m_etcd, service_factory<storage_interface>(m_ioc, 1, nullptr)) {

        m_storage_maintainer.add_monitor(m_ec_maintainer);
    }

    void run() {
        LOG_INFO() << "running coordinator service";

        boost::asio::co_spawn(
            m_ioc,
            // TODO: replace 1s with 1h
            periodic_executor(m_ioc, 1s,
                              [&]() -> coro<void> {
                                  co_return co_await license_handler(m_ioc,
                                                                     m_etcd);
                              }),
            boost::asio::detached);

        while (!m_stopped) {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this] { return m_stopped; });
        }
    }

    void stop() {
        LOG_INFO() << "stopping coordinator service";
        {
            std::lock_guard lock(m_mutex);
            m_stopped = true;
        }
        m_cv.notify_all();
        m_storage_maintainer.remove_monitor(m_ec_maintainer);
    }

private:
    etcd_manager m_etcd;
    boost::asio::io_context m_ioc;
    std::condition_variable m_cv;
    std::mutex m_mutex;
    bool m_stopped = false;

    io_context_runner m_ioc_runner;

    ec_group_maintainer m_ec_maintainer;
    service_maintainer<storage_interface> m_storage_maintainer;
};
} // namespace uh::cluster::coordinator
