#pragma once

#include <atomic>
#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <functional>
#include <stdexcept>

namespace uh::cluster {

class campaign_strategy {
public:
    virtual void pre_campaign() {}
    virtual void on_elected() {}
    virtual void post_campaign() {}
    virtual ~campaign_strategy() = default;
};

class candidate {
public:
    /*
     * @param etcd: etcd_manager instance to use for campaigning
     * @param key: key for election
     * @param candidate_name: value to set if this candidate wins election
     * @param pre_campaign: function to be called before starting campaign
     * @param on_elected: function to be called when this candidate wins
     * election
     */
    candidate(etcd_manager& etcd, const std::string& key,
              std::string candidate_name,
              std::unique_ptr<campaign_strategy> strategy = nullptr)
        : m_etcd{etcd},
          m_key{key},
          m_candidate_name{candidate_name},
          m_strategy{std::move(strategy)},
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
        if (m_strategy)
            m_strategy->pre_campaign();

        // Create key with empty value first,
        auto resp = m_etcd.create_if_empty(m_key, "");
        if (resp.is_ok()) {
            m_is_leader.store(true, std::memory_order_release);
            if (m_strategy)
                m_strategy->on_elected();
            m_etcd.put(m_key, m_candidate_name);
        }

        if (m_strategy)
            m_strategy->post_campaign();
        return resp;
    }

    void on_watch(etcd_manager::response resp) {
        try {
            switch (get_etcd_action_enum(resp.action)) {
            case etcd_action::DELETE: {
                if (!is_leader()) {
                    auto resp = campaign();
                    if (resp.is_ok()) {
                        // TODO: let's cancel this watcher here for
                        // efficiency
                        // m_watch_guard.reset();
                    }
                }
            } break;
            default:
                break;
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error trying create_if_any on candidate "
                       << m_candidate_name << ": " << e.what();
        }
    }

    etcd_manager& m_etcd;
    std::string m_key;
    std::string m_candidate_name;
    std::unique_ptr<campaign_strategy> m_strategy;
    std::function<void(void)> m_on_elected;
    std::atomic<bool> m_is_leader;
    std::optional<etcd_manager::watch_guard> m_watch_guard;
};

} // namespace uh::cluster
