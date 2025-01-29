#pragma once

#include <common/license/license.h>

#include <common/telemetry/log.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>

namespace uh::cluster {

class license_watcher {
public:
    license_watcher(etcd_manager& etcd)
        : m_etcd{etcd},
          m_wg{m_etcd.watch(etcd_license, [this](const etcd::Response& resp) {
              on_watch(resp);
          })} {

        auto license_str = m_etcd.get(etcd_license);
        if (!license_str.empty()) {
            parse_and_save(license_str);
        } else {
            LOG_INFO()
                << "The coordinator has not yet updated the license string";
        }
    }
    license& get_license() { return *m_license.load(); }

private:
    void on_watch(const etcd::Response& resp) {
        LOG_INFO() << "License updated";

        const auto& license_str = resp.value().as_string();
        parse_and_save(license_str);
    }

    void parse_and_save(std::string_view license_str) {
        auto lic = license::create(license_str, license::verify::SKIP_VERIFY);
        LOG_INFO() << lic.to_string();
        m_license.store(std::make_shared<license>(lic));
    }

    etcd_manager& m_etcd;
    etcd_manager::watch_guard m_wg;
    std::atomic<std::shared_ptr<license>> m_license;
};

} // namespace uh::cluster
