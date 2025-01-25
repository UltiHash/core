#pragma once

#include "common/telemetry/log.h"
#include "internal/payg.h"

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>

namespace uh::cluster::lic {

class payg_watcher {
public:
    using callback_t = std::function<void(const payg& license)>;
    payg_watcher(etcd_manager& etcd, callback_t&& callback)
        : m_etcd{etcd},
          m_callback{std::forward<callback_t>(callback)},
          m_wg{m_etcd.watch(etcd_payg_license,
                            [this](etcd::Response) { on_watch(); })} {

        try {
            auto handler = payg_handler(m_etcd.get(etcd_payg_license),
                                        payg_handler::verify::SKIP_VERIFY);
            auto license = handler.get();
            m_license.store(std::make_shared<payg>(license));
        } catch (nlohmann::json::parse_error& e) {
            LOG_ERROR() << "Invalid JSON string: " << e.what();
        }
    }
    payg get() { return *m_license.load(); }

private:
    void on_watch() {
        LOG_INFO() << "License updated";
        auto handler = payg_handler(m_etcd.get(etcd_payg_license),
                                    payg_handler::verify::SKIP_VERIFY);
        LOG_INFO() << handler.to_string();
        auto license = handler.get();
        m_license.store(std::make_shared<payg>(license));
        m_callback(license);
    }

    etcd_manager& m_etcd;
    callback_t m_callback;
    etcd_manager::watch_guard m_wg;
    std::atomic<std::shared_ptr<payg>> m_license;
};

} // namespace uh::cluster::lic

// template <typename Struct, typename MemberType>
// MemberType get_member_type(MemberType Struct::*);
//
// template <auto Member> class payg_manager {
// public:
//     using MemberType = decltype(get_member_type(Member));
//
//     payg_manager(etcd_manager& etcd, std::atomic<MemberType>& storage_cap)
//         : m_etcd{etcd} {
//
//         storage_cap.store(
//             m_etcd.get(etcd_payg_license);
//         m_wg = m_etcd.watch(etcd_payg_license,
//                             [&](etcd::Response) {
//             storage_cap.store(m_etcd.get(etcd_payg_license));
//                             });
//     }
//
// private:
//     etcd_manager& m_etcd;
//     etcd_manager::watch_guard m_wg;
// };
