#pragma once

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/license/backend_client.h>
#include <common/license/exp_backoff.h>
#include <common/license/license.h>
#include <common/license/license_updater.h>
#include <common/types/common_types.h>

namespace uh::cluster {

class license_updater {

public:
    template <typename T>
    license_updater(boost::asio::io_context& ioc, etcd_manager& etcd,
                    T&& client)
        : m_ioc{ioc},
          m_etcd{etcd},
          m_backend_client{std::make_unique<T>(std::forward<T>(client))} {}

    void start_update() {}

    coro<void> update() {
        auto backoff = exponential_backoff<std::string>{m_ioc, 7, 100, 200};
        try {
            LOG_DEBUG() << "Fetching license ...";
            auto str = co_await backoff.run([&]() -> coro<std::string> {
                co_return co_await m_backend_client->get_license();
            });

            auto lic = license::create(str);

            LOG_INFO() << "license loaded for " << lic.customer_id;
            LOG_INFO() << " -- license type: "
                       << magic_enum::enum_name(lic.license_type);
            LOG_INFO() << " -- storage size: " << lic.storage_cap_gib
                       << " GiBs";

            m_etcd.put(etcd_license, lic.to_string());

            LOG_DEBUG() << "License updated";

        } catch (const std::runtime_error& e) {
            LOG_ERROR() << "License check failed: " << e.what();
            license lic;
            m_etcd.put(etcd_license, lic.to_string());
        } catch (...) {
        }
        co_return;
    }
    coro<void> periodic_update(std::chrono::seconds interval) {
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

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    std::unique_ptr<backend_client> m_backend_client;
};

} // namespace uh::cluster
