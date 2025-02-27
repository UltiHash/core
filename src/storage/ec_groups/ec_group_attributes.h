#pragma once
#include "common/etcd/namespace.h"
#include "common/etcd/utils.h"
#include "common/utils/time_utils.h"
#include <magic_enum/magic_enum.hpp>

namespace uh::cluster {

enum etcd_ec_group_attributes {
    EC_GROUP_SIZE,
    EC_GROUP_STATUS,
    EC_GROUP_EC_NODES,
};

constexpr std::array<
    std::pair<uh::cluster::etcd_ec_group_attributes, const char*>, 3>
    string_by_ec_group_attribute = {{
        {uh::cluster::EC_GROUP_SIZE, "group_size"},
        {uh::cluster::EC_GROUP_STATUS, "group_status"},
        {uh::cluster::EC_GROUP_EC_NODES, "group_ec_nodes"},
    }};

constexpr const char* get_etcd_ec_group_attribute_string(
    const uh::cluster::etcd_ec_group_attributes& param) {
    for (const auto& entry : string_by_ec_group_attribute) {
        if (entry.first == param)
            return entry.second;
    }

    throw std::invalid_argument("invalid etcd parameter");
}

constexpr uh::cluster::etcd_ec_group_attributes
get_etcd_ec_group_attribute_enum(const std::string& param) {
    for (const auto& entry : string_by_ec_group_attribute) {
        if (entry.second == param)
            return entry.first;
    }

    throw std::invalid_argument("invalid etcd parameter");
}

inline static std::string get_ec_group_path(int group_id) {
    return etcd_ec_groups_key_prefix + std::to_string(group_id);
}

inline static std::string
get_ec_group_attribute_path(size_t group_id, etcd_ec_group_attributes attr) {
    return etcd_ec_groups_key_prefix + std::to_string(group_id) + "/" +
           get_etcd_ec_group_attribute_string(attr);
}

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
