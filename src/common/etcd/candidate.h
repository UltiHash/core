#pragma once

#include <atomic>
#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <common/utils/strings.h>
#include <functional>

namespace uh::cluster {

class candidate {
public:
    using callback_t = std::function<void(bool leader)>;
    using id_t = int; // to represent -1 as empty
    constexpr static id_t staging_id = -1;

    /*
     * NOTE: On a leader, the callback is called before announcing itself as a
     * leader.
     */
    candidate(etcd_manager& etcd, const std::string& key, id_t id,
              callback_t callback = nullptr)
        : m_etcd{etcd},
          m_key{key},
          m_id{id},
          m_callback{std::move(callback)},
          m_is_leader{false} {

        auto resp = campaign();
        if (resp.is_ok())
            return;

        m_watch_guard = m_etcd.watch(
            m_key, [this](etcd_manager::response resp) { on_watch(resp); },
            resp.index() + 1);
    }

    ~candidate() {
        m_watch_guard.reset();
        m_etcd.rm(m_key);
    }

    auto is_leader() const {
        return m_is_leader.load(std::memory_order_acquire);
    }

private:
    auto campaign() -> etcd::Response {
        // Create key with -1 first,
        auto resp = m_etcd.create_if_empty(m_key, serialize<int>(staging_id));

        if (m_callback)
            m_callback(resp.is_ok());

        if (resp.is_ok()) {
            m_is_leader.store(true, std::memory_order_release);
            m_etcd.put(m_key, serialize<int>(m_id));
        }

        return resp;
    }

    void on_watch(etcd_manager::response resp) {
        try {
            switch (get_etcd_action_enum(resp.action)) {
            case etcd_action::DELETE: {
                auto resp = campaign();
                if (resp.is_ok()) {
                    // TODO: let's cancel this watcher here for efficiency
                }
            } break;
            default:
                break;
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error trying create_if_any on candidate "
                       << std::to_string(m_id) << ": " << e.what();
        }
    }

    etcd_manager& m_etcd;
    std::string m_key;
    id_t m_id;
    callback_t m_callback;
    std::atomic<bool> m_is_leader;
    std::optional<etcd_manager::watch_guard> m_watch_guard;
};

} // namespace uh::cluster
