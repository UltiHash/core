#pragma once

#include <storage/group/state.h>

#include <common/telemetry/log.h>

#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/utils/strings.h>

namespace uh::cluster::storage_group {

class state_watcher {
public:
    using callback_t = std::function<void(std::string_view)>;
    state_watcher(etcd_manager& etcd, callback_t callback = nullptr)
        : m_etcd{etcd},
          m_wg{m_etcd.watch(
              get_storage_group_state_path(),
              [this](const etcd::Response& resp) { on_watch(resp); })} {
        // TODO: m_states.reserve(); according to the number of groups
    }
    std::shared_ptr<state> get_state(int id) const {
        return m_states.at(id).load();
    }

private:
    void on_watch(const etcd::Response& resp) {
        try {
            LOG_INFO() << "Watcher has detected a group state update on group "
                       << resp.value().key();
            auto group_id = stoul(resp.value().key());
            parse_and_save(group_id, resp.value().as_string());

            LOG_INFO() << "Modified storage_group::state for group saved";

        } catch (const std::exception& e) {
            LOG_WARN() << "error updating storage_group::state: " << e.what();
        }
    }

    void parse_and_save(int group_id, std::string_view str) {
        auto s = state::create(str);
        m_states.at(group_id).store(std::make_shared<state>(s));
    }

    etcd_manager& m_etcd;
    etcd_manager::watch_guard m_wg;
    std::vector<std::atomic<std::shared_ptr<state>>> m_states;
};

} // namespace uh::cluster::storage_group
