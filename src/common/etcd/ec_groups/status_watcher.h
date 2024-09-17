#ifndef STATUS_WATCHER_H
#define STATUS_WATCHER_H
#include "common/telemetry/log.h"
#include "ec_group_attributes.h"

namespace uh::cluster {

class status_watcher {
public:
    status_watcher(ec_group_attributes& attributes,
                   std::atomic<ec_status>& status)
        :

          m_status(status),
          m_attributes(attributes),
          m_watcher(
              m_attributes.etcd_client(),
              get_ec_group_attribute_path(m_attributes.group_id(),
                                          EC_GROUP_STATUS),
              [this](const etcd::Response& response) {
                  return handle_state_changes(response);
              },
              true) {
        if (auto stat = m_attributes.get_status(); stat) {
            m_status = *stat;
        }
    }

    ~status_watcher() { m_watcher.Cancel(); }

private:
    void handle_state_changes(const etcd::Response& response) {
        try {

            std::lock_guard<std::mutex> lk(m_mutex);

            switch (get_etcd_action_enum(response.action())) {
            case etcd_action::create:
            case etcd_action::set:
                m_status = response_to_status(response);
                break;
            case etcd_action::erase:
                break;
            }
        } catch (const std::exception& e) {
            LOG_WARN() << "error while handling status state change: "
                       << e.what();
        }
    }

    std::atomic<ec_status>& m_status;
    ec_group_attributes& m_attributes;
    etcd::Watcher m_watcher;
    std::mutex m_mutex;
};
} // end namespace uh::cluster

#endif // STATUS_WATCHER_H
