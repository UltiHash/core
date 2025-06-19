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
          m_callback{std::move(callback)},
          m_license{std::make_shared<license>()},
          m_wg{m_etcd.watch(
              etcd_license_key,
              [this](etcd_manager::response resp) { on_watch(resp); })} {}
    std::shared_ptr<license> get_license() { return m_license.load(); }

private:
    void on_watch(etcd_manager::response resp) {
        try {
            LOG_INFO() << "Watcher has detected a license update";

            switch (get_etcd_action_enum(resp.action)) {
            case etcd_action::GET:
            case etcd_action::CREATE:
            case etcd_action::SET: {
                LOG_INFO() << "Modified license saved";
                parse_and_save(resp.value);
                break;
            }
            case etcd_action::DELETE:
            default:
                LOG_INFO() << "License deleted";
                m_license.store(std::make_shared<license>());
                break;
            }

            if (m_callback) {
                m_callback(resp.value);
            }
        } catch (const std::exception& e) {
            m_license.store(std::make_shared<license>());
            LOG_WARN() << "error updating license: " << e.what();
        }
    }

    void parse_and_save(const std::string& license_str) {
        auto lic = license::create(license_str, license::verify::SKIP_VERIFY);

        LOG_INFO() << "license loaded for " << lic.customer_id;
        LOG_INFO() << " -- license type: "
                   << magic_enum::enum_name(lic.license_type);
        LOG_INFO() << " -- storage size: " << lic.storage_cap_gib << " GiBs";

        m_license.store(std::make_shared<license>(lic));
    }

    etcd_manager& m_etcd;
    callback_t m_callback;
    std::atomic<std::shared_ptr<license>> m_license;
    etcd_manager::watch_guard m_wg;
};

} // namespace uh::cluster
