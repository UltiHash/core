#pragma once

#include <atomic>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <stop_token>
#include <thread>

namespace uh::cluster {

class observer {
public:
    /*
     * @param etcd: etcd_manager instance to use for observing
     * @param name: name of the election
     */
    observer(etcd_manager& etcd, const std::string& name)
        : m_etcd{etcd},
          m_name{name},
          m_leader{std::make_shared<std::string>()},
          worker_thread{&observer::worker, this} {}

    /*
     * @return the storage ID of the leader.
     */
    auto get() const { return *(m_leader.load(std::memory_order_acquire)); }

private:
    void worker(std::stop_token token) {
        m_observer = m_etcd.observe(m_name);
        while (token.stop_requested() == false) {
            etcd::Response resp = m_observer->WaitOnce();

            if (resp.is_grpc_timeout()) {
                LOG_DEBUG() << "timeout on observer";
                continue;
            }

            if (!resp.is_ok()) {
                LOG_WARN() << "observe failed";
                m_leader.store(std::make_shared<std::string>(),
                               std::memory_order_release);
                continue;
            }

            LOG_INFO() << "observe " << resp.value().key()
                       << " as the leader: " << resp.value().as_string();

            m_leader.store(
                std::make_shared<std::string>(resp.value().as_string()),
                std::memory_order_release);
        }
    }

    etcd_manager& m_etcd;
    std::string m_name;
    std::atomic<std::shared_ptr<std::string>> m_leader;
    std::jthread worker_thread;
    std::unique_ptr<etcd::SyncClient::Observer> m_observer;
};

static_assert(
    std::is_same_v<decltype(std::declval<observer>().get()), std::string>,
    "Return type is not std::string");

} // namespace uh::cluster
