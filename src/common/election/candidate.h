#pragma once

#include <atomic>
#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <functional>
#include <stdexcept>

namespace uh::cluster {

class candidate {
public:
    /*
     * @param etcd: etcd_manager instance to use for campaigning
     * @param election_name: name of the election
     * @param candidate_name: value to set if this candidate wins the election
     * @param callback: function to call when this candidate wins the election
     */
    candidate(etcd_manager& etcd, const std::string& election_name,
              std::string candidate_name,
              std::function<void(void)> callback = nullptr)
        : m_etcd{etcd},
          m_election_name{election_name},
          m_candidate_name{candidate_name},
          m_callback{std::move(callback)},
          m_is_leader{false} {

        // Create key with empty value first,
        auto resp = m_etcd.create_if_empty(m_election_name, "");
        if (resp.is_ok()) {
            m_is_leader.store(true, std::memory_order_release);
            if (m_callback)
                std::move(m_callback)();
            // Set the candidate name
            m_etcd.put(m_election_name, m_candidate_name);
            return;
        }
        if (resp.error_code() != etcdv3::ERROR_COMPARE_FAILED) {
            throw std::runtime_error("modify_if returns error code: " +
                                     std::to_string(resp.error_code()));
        }

        m_watch_guard = m_etcd.watch(
            m_election_name,
            [this](etcd_manager::response resp) {
                try {
                    switch (get_etcd_action_enum(resp.action)) {
                    case etcd_action::DELETE: {
                        auto resp = m_etcd.create_if_empty(m_election_name, "");
                        if (resp.is_ok()) {
                            m_is_leader.store(true, std::memory_order_release);
                            if (m_callback)
                                std::move(m_callback)();
                            m_etcd.put(m_election_name, m_candidate_name);
                            // TODO: let's remove this watcher here for
                            // efficiency
                        } else if (resp.error_code() !=
                                   etcdv3::ERROR_COMPARE_FAILED) {
                            throw std::runtime_error(
                                "modify_if returns error code: " +
                                std::to_string(resp.error_code()));
                        }
                        break;
                    }
                    default:
                        break;
                    }
                } catch (const std::exception& e) {
                    LOG_WARN() << "error trying create_if_any on candidate "
                               << m_candidate_name << ": " << e.what();
                }
            },
            resp.index() + 1);
    }

    ~candidate() { m_etcd.rm(m_election_name); }

    auto is_leader() const {
        return m_is_leader.load(std::memory_order_acquire);
    }

private:
    etcd_manager& m_etcd;
    std::string m_election_name;
    std::string m_candidate_name;
    std::function<void(void)> m_callback;
    std::atomic<bool> m_is_leader;
    etcd_manager::watch_guard m_watch_guard;
};

} // namespace uh::cluster
