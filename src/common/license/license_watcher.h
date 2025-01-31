#pragma once

#include <common/license/license.h>

#include <common/telemetry/log.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>

namespace uh::cluster {

class license_watcher {
public:
    using callback_t = std::function<void(std::string_view)>;
    license_watcher(etcd_manager& etcd, callback_t callback = nullptr)
        : m_etcd{etcd},
          m_wg{m_etcd.watch(
              etcd_license,
              [this](const etcd::Response& resp) { on_watch(resp); })},
          m_license{std::make_shared<license>()},
          m_callback{std::move(callback)} {

        auto license_str = m_etcd.get(etcd_license);
        if (!license_str.empty()) {
            parse_and_save(license_str);

            LOG_INFO() << "License saved";
        } else {
            LOG_INFO()
                << "The coordinator has not yet updated the license string";
        }
    }
    license& get_license() { return *m_license.load(); }

private:
    void on_watch(const etcd::Response& resp) {
        LOG_INFO() << "Watcher has detected a license update";

        const auto& license_str = resp.value().as_string();
        parse_and_save(license_str);

        LOG_INFO() << "Modified license saved";

        if (m_callback) {
            m_callback(license_str);
        }
    }

    void parse_and_save(std::string_view license_str) {
        auto lic = license::create(license_str, license::verify::SKIP_VERIFY);

        LOG_INFO() << "license loaded for " << lic.customer_id
                   << " -- storage size: " << lic.storage_cap_gib << " bytes";

        m_license.store(std::make_shared<license>(lic));
    }

    etcd_manager& m_etcd;
    etcd_manager::watch_guard m_wg;
    std::atomic<std::shared_ptr<license>> m_license;
    callback_t m_callback;
};

} // namespace uh::cluster
