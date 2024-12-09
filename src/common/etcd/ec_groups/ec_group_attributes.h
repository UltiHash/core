#ifndef EC_GROUP_ATTRIBUTES_H
#define EC_GROUP_ATTRIBUTES_H
#include "common/etcd/namespace.h"
#include "common/utils/time_utils.h"
#include <etcd/SyncClient.hpp>
#include <etcd/Watcher.hpp>
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

    void clear() { m_etcd_client.rmdir(get_ec_group_path(m_gid)); }

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

    [[nodiscard]] etcd::SyncClient& etcd_client() const noexcept {
        return m_etcd_client;
    }

private:
    void set_attribute(etcd_ec_group_attributes attr,
                       const std::string& value) {
        m_etcd_client.set(get_ec_group_attribute_path(m_gid, attr), value);
    }

    etcd::SyncClient& m_etcd_client;
    const size_t m_gid;
};

} // end namespace uh::cluster

#endif // EC_GROUP_ATTRIBUTES_H
