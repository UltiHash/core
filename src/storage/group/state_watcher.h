#pragma once

#include <storage/group/state.h>

#include <common/telemetry/log.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>

namespace uh::cluster::storage_group {

class state_watcher {
public:
    using callback_t = std::function<void(state*)>;
    state_watcher(etcd_manager& etcd, size_t group_id,
                  callback_t callback = nullptr)
        : m_etcd{etcd},
          m_callback{std::move(callback)},
          m_wg{m_etcd.watch(
              get_storage_group_state_path(group_id),
              [this](etcd_manager::response resp) { on_watch(resp); })} {}

    std::shared_ptr<state> get_state() const {
        return m_states.load(std::memory_order_acquire);
    }

private:
    void on_watch(etcd_manager::response resp) {
        try {
            LOG_INFO() << "Watcher has detected a group state update on group "
                       << resp.value;

            switch (get_etcd_action_enum(resp.action)) {
            case etcd_action::GET:
            case etcd_action::CREATE:
            case etcd_action::SET: {
                auto s = state::create(resp.value);
                m_states.store(std::make_shared<state>(s));

                LOG_INFO() << "Modified storage_group::state for group saved";
                break;
            }
            case etcd_action::DELETE:
            default:
                LOG_INFO() << "License deleted";
                m_states.store(std::make_shared<state>());
                break;
            }

            if (m_callback)
                m_callback(get_state().get());

        } catch (const std::exception& e) {
            LOG_WARN() << "error updating storage_group::state: " << e.what();
        }
    }

    etcd_manager& m_etcd;
    callback_t m_callback;
    std::atomic<std::shared_ptr<state>> m_states;
    etcd_manager::watch_guard m_wg;
};

} // namespace uh::cluster::storage_group
