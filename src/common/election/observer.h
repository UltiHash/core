#pragma once

#include <atomic>
#include <common/etcd/namespace.h>
#include <common/etcd/utils.h>
#include <common/telemetry/log.h>
#include <functional>

namespace uh::cluster {

class observer {
public:
    /*
     * @param etcd: etcd_manager instance to use for campaigning
     * @param election_name: name of the election
     * @param value: value to set if this observer wins the election
     * @param callback: function to call when this observer wins the election
     */
    observer(etcd_manager& etcd, const std::string& election_name,
             std::function<void(void)> callback = nullptr)
        : m_etcd{etcd},
          m_election_name{election_name},
          m_callback{std::move(callback)},
          m_leader{std::make_shared<std::string>()},
          m_watch_guard{m_etcd.watch(
              m_election_name, [this](etcd_manager::response resp) {
                  try {
                      // NOTE: Do not handle CREATE, as the candidate creates
                      // the key with an empty value as a temporary placeholder.
                      switch (get_etcd_action_enum(resp.action)) {
                      case etcd_action::SET: {
                          auto value = m_etcd.get(m_election_name);
                          if (!value.empty()) {
                              LOG_INFO() << "observe " << m_election_name
                                         << " as the leader: " << value;

                              m_leader.store(
                                  std::make_shared<std::string>(value),
                                  std::memory_order_release);

                              if (m_callback)
                                  m_callback();
                          }
                          break;
                      }
                      case etcd_action::DELETE: {
                          m_leader.store(std::make_shared<std::string>(),
                                         std::memory_order_release);
                          break;
                      }
                      default:
                          break;
                      }
                  } catch (const std::exception& e) {
                      LOG_ERROR() << "Error in observer callback: " << e.what();

                      m_leader.store(std::make_shared<std::string>(),
                                     std::memory_order_release);
                  }
              })} {}

    auto get() const { return *(m_leader.load(std::memory_order_acquire)); }

private:
    etcd_manager& m_etcd;
    std::string m_election_name;
    std::function<void(void)> m_callback;
    std::atomic<std::shared_ptr<std::string>> m_leader;
    etcd_manager::watch_guard m_watch_guard;
};

/*
 * @return the storage ID of the leader.
 */

static_assert(
    std::is_same_v<decltype(std::declval<observer>().get()), std::string>,
    "Return type is not std::string");

} // namespace uh::cluster
