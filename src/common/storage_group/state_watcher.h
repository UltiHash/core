#pragma once

#include <common/storage_group/state.h>

#include <common/telemetry/log.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>

namespace uh::cluster::storage_group {

class state_watcher {
public:
    using callback_t = std::function<void(std::string_view)>;
    state_watcher(etcd_manager& etcd, callback_t callback = nullptr)
        : m_etcd{etcd},
          m_wg{m_etcd.watch(
              etcd_license_key,
              [this](const etcd::Response& resp) { on_watch(resp); })},
          m_state{std::make_shared<state>()},
          m_callback{std::move(callback)} {

        auto cfg_str = m_etcd.get(etcd_license_key);
        if (!cfg_str.empty()) {
            parse_and_save(cfg_str);

            LOG_INFO() << "storage_group::state saved";
        } else {
            LOG_INFO() << "The coordinator has not yet updated the "
                          "storage_group::state string";
        }
    }
    std::shared_ptr<state> get_state() const { return m_state.load(); }

private:
    void on_watch(const etcd::Response& resp) {
        try {
            LOG_INFO() << "Watcher has detected a storage_group::state update";

            const auto& cfg_str = resp.value().as_string();
            parse_and_save(cfg_str);

            LOG_INFO() << "Modified storage_group::state saved";

            if (m_callback) {
                m_callback(cfg_str);
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error updating storage_group::state: " << e.what();
        }
    }

    void parse_and_save(std::string_view cfg_str) {
        auto lic = state::create(cfg_str);
        m_state.store(std::make_shared<state>(lic));
    }

    etcd_manager& m_etcd;
    etcd_manager::watch_guard m_wg;
    std::atomic<std::shared_ptr<state>> m_state;
    callback_t m_callback;
};

} // namespace uh::cluster::storage_group
