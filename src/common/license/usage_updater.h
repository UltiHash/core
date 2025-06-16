#pragma once

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/license/backend_client.h>
#include <common/license/exp_backoff.h>
#include <common/license/license_updater.h>
#include <common/license/usage.h>
#include <common/types/common_types.h>

#include <nlohmann/json.hpp>
#include <optional>

namespace uh::cluster {

class usage_updater {

public:
    template <typename T>
    usage_updater(boost::asio::io_context& ioc, usage& usage,
                  license_updater& license, T&& client)
        : m_ioc{ioc},
          m_usage{usage},
          m_license(license),
          m_backend_client{std::make_unique<T>(std::forward<T>(client))} { }

    coro<void>
    update(std::chrono::time_point<std::chrono::system_clock> full_hour) {
        auto backoff = exponential_backoff<std::string>{m_ioc, 7, 100, 200};
        try {
            LOG_DEBUG() << "Sending usage data ...";
            std::string json_str =
                co_await generate_json(full_hour - 1h, full_hour);
            co_await backoff.run([&, json_str]() -> coro<std::string> {
                co_return co_await m_backend_client->post_usage(
                    std::move(json_str));
            });
        } catch (const std::runtime_error& e) {
            LOG_ERROR() << "Sending usage data failed: " << e.what();
        } catch (...) {
            LOG_ERROR() << "Sending usage data failed: unknown error";
        }
        co_return;
    }

    coro<void> hourly_update() {
        std::shared_ptr<license> lic = m_license.current();

        if (!m_last_type) {
            m_last_type = lic ? lic->license_type : license::NONE;
        }

        auto now = std::chrono::system_clock::now();
        if (!m_next_full_hour) {
            m_next_full_hour = std::chrono::ceil<std::chrono::hours>(now);
        }

        if (now < *m_next_full_hour) {
            co_return;
        }

        if ((lic && lic->license_type == license::PREMIUM) ||
            m_last_type == license::PREMIUM) {
            co_await update(*m_next_full_hour);
        }

        if (lic) {
            m_last_type = lic->license_type;
        }

        m_next_full_hour = std::chrono::ceil<std::chrono::hours>(
            std::chrono::system_clock::now());
    }

    static constexpr auto POLL_INTERVAL = std::chrono::seconds(5);

private:
    boost::asio::io_context& m_ioc;
    usage& m_usage;
    license_updater& m_license;
    std::unique_ptr<backend_client> m_backend_client;

    std::optional<license::type> m_last_type;
    std::optional<std::chrono::time_point<std::chrono::system_clock>> m_next_full_hour;

    coro<std::string> generate_json(const utc_time& interval_infimum,
                                    const utc_time& interval_supremum) {
        std::size_t storage_usage = co_await m_usage.get_usage_for_interval(
            interval_infimum, interval_supremum);

        nlohmann::json json_obj = {
            {"version", "v1"},
            {"storage_usage", storage_usage},
            {"interval_infimum", std::format("{0:%F %T}", interval_infimum)},
            {"interval_supremum", std::format("{0:%F %T}", interval_supremum)}};

        co_return json_obj.dump();
    }
};

} // namespace uh::cluster
