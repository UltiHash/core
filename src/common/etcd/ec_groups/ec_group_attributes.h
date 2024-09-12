#ifndef EC_GROUP_ATTRIBUTES_H
#define EC_GROUP_ATTRIBUTES_H
#include "common/etcd/namespace.h"
#include "common/utils/time_utils.h"
#include "third-party/etcd-cpp-apiv3/etcd/Watcher.hpp"
#include <etcd/SyncClient.hpp>
#include <magic_enum/magic_enum.hpp>

namespace uh::cluster {

enum ec_status {
    empty,
    degraded,
    recovering,
    healthy,
    failed_recovery,
};

static ec_status response_to_status(const etcd::Response& response) {
    const auto& value = response.value().as_string();
    const auto stat = magic_enum::enum_cast<ec_status>(value);
    if (!stat)
        throw std::runtime_error("invalid ec status");
    return *stat;
}

class ec_group_attributes {
public:
    ec_group_attributes(size_t gid, etcd::SyncClient& etcd_client)
        : m_etcd_client(etcd_client),
          m_gid(gid) {}

    ec_group_attributes(const ec_group_attributes&) = delete;

    void set_group_size(size_t size) {
        set_attribute(EC_GROUP_SIZE, std::to_string(size));
    }

    void set_ec_nodes(size_t count) {
        set_attribute(EC_GROUP_EC_NODES, std::to_string(count));
    }

    void set_status(ec_status status) {
        set_attribute(EC_GROUP_STATUS,
                      std::string(magic_enum::enum_name(status)));
    }

    std::optional<ec_status> get_status() {
        auto resp = wait_for_success(ETCD_TIMEOUT, ETCD_RETRY_INTERVAL, [this] {
            return m_etcd_client.get(
                get_ec_group_attribute_path(m_gid, EC_GROUP_STATUS));
        });
        if (resp.is_ok()) {
            return response_to_status(resp);
        }
        return std::nullopt;
    }

    [[nodiscard]] size_t group_id() const noexcept { return m_gid; }

    etcd::SyncClient& etcd_client() const noexcept { return m_etcd_client; }

private:
    void set_attribute(etcd_ec_group_attributes attr,
                       const std::string& value) {
        m_etcd_client.set(get_ec_group_attribute_path(m_gid, attr), value);
    }

    etcd::SyncClient& m_etcd_client;
    const size_t m_gid;
};

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
    bool m_stop = false;
    std::mutex m_mutex;
};
} // end namespace uh::cluster

#endif // EC_GROUP_ATTRIBUTES_H
