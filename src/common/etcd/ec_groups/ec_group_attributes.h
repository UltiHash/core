#pragma once
#include "common/etcd/namespace.h"
#include "common/etcd/utils.h"
#include "common/utils/time_utils.h"
#include <magic_enum/magic_enum.hpp>

namespace uh::cluster {

enum ec_status {
    empty,
    degraded,
    recovering,
    healthy,
    failed_recovery,
};

static ec_status response_to_status(std::string response_value) {
    const auto stat = magic_enum::enum_cast<ec_status>(response_value);
    if (!stat)
        throw std::runtime_error("invalid ec status");
    return *stat;
}

class ec_group_attributes {
public:
    ec_group_attributes(size_t gid, etcd_manager& etcd)
        : m_etcd(etcd),
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

    void clear() { m_etcd.rmdir(get_ec_group_path(m_gid)); }

    std::optional<ec_status> get_status() {
        try {
            return response_to_status(m_etcd.get(
                get_ec_group_attribute_path(m_gid, EC_GROUP_STATUS)));
        } catch (...) {
            return std::nullopt;
        }
    }

    [[nodiscard]] size_t group_id() const noexcept { return m_gid; }

    [[nodiscard]] etcd_manager& etcd_client() const noexcept { return m_etcd; }

private:
    void set_attribute(etcd_ec_group_attributes attr,
                       const std::string& value) {
        m_etcd.put(get_ec_group_attribute_path(m_gid, attr), value);
    }

    etcd_manager& m_etcd;
    const size_t m_gid;
};

} // end namespace uh::cluster
