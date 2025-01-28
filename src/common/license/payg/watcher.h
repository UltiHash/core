#pragma once

#include <common/license/payg/payg.h>

#include <common/telemetry/log.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>

namespace uh::cluster {

class payg_watcher {
public:
    payg_watcher(etcd_manager& etcd)
        : m_etcd{etcd},
          m_wg{m_etcd.watch(
              etcd_payg_license,
              [this](const etcd::Response& resp) { on_watch(resp); })} {

        auto license_str = m_etcd.get(etcd_payg_license);
        if (!license_str.empty()) {
            parse_and_save(license_str);
        } else {
            LOG_INFO()
                << "The coordinator has not yet updated the license string";
        }
    }
    payg_license& get_license() { return *m_license.load(); }

private:
    void on_watch(const etcd::Response& resp) {
        LOG_INFO() << "License updated";

        const auto& license_str = resp.value().as_string();
        auto license = parse_and_save(license_str);
    }

    payg_license parse_and_save(std::string_view license_str) {
        auto license = payg_license::create(license_str,
                                            payg_license::verify::SKIP_VERIFY);
        LOG_INFO() << license.to_string();
        m_license.store(std::make_shared<payg_license>(license));
        return license;
    }

    etcd_manager& m_etcd;
    etcd_manager::watch_guard m_wg;
    std::atomic<std::shared_ptr<payg_license>> m_license;
};

} // namespace uh::cluster
