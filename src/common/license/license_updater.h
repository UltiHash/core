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

    coro<void> update() {
        auto backoff = exponential_backoff<std::string>{m_ioc, 7, 100, 200};
        try {
            LOG_DEBUG() << "Fetching license ...";
            auto str = co_await backoff.run([&]() -> coro<std::string> {
                co_return co_await m_backend_client->get_license();
            });

            auto lic = std::make_shared<license>(license::create(str));

            LOG_INFO() << "license loaded for " << lic->customer_id;
            LOG_INFO() << " -- license type: "
                       << magic_enum::enum_name(lic->license_type);
            LOG_INFO() << " -- storage size: " << lic->storage_cap_gib
                       << " GiBs";

            m_etcd.put(etcd_license_key, lic->to_string());
            m_license = std::move(lic);

            LOG_DEBUG() << "License updated";

        } catch (const std::runtime_error& e) {
            LOG_ERROR() << "License check failed: " << e.what();

            std::shared_ptr<license> lic = std::make_shared<license>();
            m_etcd.put(etcd_license_key, lic->to_string());
            m_license = std::move(lic);
        } catch (...) {
            LOG_ERROR() << "License check failed: unknown error";
        }

        co_return;
    }

    std::shared_ptr<license> current() const { return m_license; }

private:
    boost::asio::io_context& m_ioc;
    etcd_manager& m_etcd;
    std::unique_ptr<backend_client> m_backend_client;
    std::atomic<std::shared_ptr<license>> m_license;
};

} // namespace uh::cluster
