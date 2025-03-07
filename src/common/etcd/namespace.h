#pragma once

#define NAMESPACE "uh"

#include "common/utils/common.h"
#include <filesystem>
#include <map>

namespace uh::cluster {

static constexpr const char* etcd_watchdog = "/" NAMESPACE "/watchdog/";

static constexpr const char* etcd_services_key_prefix =
    "/" NAMESPACE "/services/";
static constexpr const char* etcd_ec_groups_key_prefix =
    "/" NAMESPACE "/ec-group/";

static constexpr const char* etcd_global_lock_key =
    "/" NAMESPACE "/config/class/cluster/lock";
static constexpr const char* etcd_current_id_prefix_key =
    "/" NAMESPACE "/config/class/cluster/current_id/";
static constexpr const char* etcd_license = "/" NAMESPACE "/config/license";

enum class etcd_action : uint8_t {
    create = 0,
    set,
    erase,
};

inline etcd_action get_etcd_action_enum(const std::string& action_str) {
    static const std::map<std::string, etcd_action> etcd_action = {
        {"create", etcd_action::create},
        {"set", etcd_action::set},
        {"delete", etcd_action::erase},
    };

    if (const auto f = etcd_action.find(action_str); f != etcd_action.cend())
        return f->second;

    throw std::invalid_argument("invalid etcd action");
}

enum etcd_service_attributes {
    ENDPOINT_HOST,
    ENDPOINT_PORT,
    ENDPOINT_PID,
    STORAGE_FREE_SPACE,
    STORAGE_LOAD,
};

enum etcd_ec_group_attributes {
    EC_GROUP_SIZE,
    EC_GROUP_STATUS,
    EC_GROUP_EC_NODES,
};

constexpr std::array<
    std::pair<uh::cluster::etcd_service_attributes, const char*>, 5>
    string_by_service_attribute = {{
        {uh::cluster::ENDPOINT_HOST, "endpoint_host"},
        {uh::cluster::ENDPOINT_PORT, "endpoint_port"},
        {uh::cluster::ENDPOINT_PID, "endpoint_pid"},
        {uh::cluster::STORAGE_FREE_SPACE, "storage_free_space"},
        {uh::cluster::STORAGE_LOAD, "storage_load"},
    }};

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

inline static std::string get_service_root_path(role r) {
    return etcd_services_key_prefix + get_service_string(r);
}

inline static std::string get_announced_root(role r) {
    return get_service_root_path(r) + "/announced/";
}

inline static std::string get_announced_path(role r, unsigned long id) {
    return get_announced_root(r) + std::to_string(id);
}

inline static std::string get_attributes_path(role r, unsigned long id) {
    return get_service_root_path(r) + "/attributes/" + std::to_string(id) + "/";
}

inline static std::string get_attribute_key(const std::string& path) {
    return std::filesystem::path(path).filename();
}

inline static unsigned long
get_announced_id(const std::string& announced_path) {
    const auto id = std::filesystem::path(announced_path).filename().string();
    return std::stoul(id);
}

inline static unsigned long
get_attribute_id(const std::string& announced_path) {
    const auto id =
        std::filesystem::path(announced_path).parent_path().filename().string();
    return std::stoul(id);
}

inline static bool service_announced_path(const std::string& path) {
    return std::filesystem::path(path).parent_path().filename() == "announced";
}

inline static bool service_attributes_path(const std::string& path) {
    return std::filesystem::path(path).parent_path().parent_path().filename() ==
           "attributes";
}

inline static unsigned long get_id(const std::string& path) {
    if (service_announced_path(path)) {
        return get_announced_id(path);
    } else if (service_attributes_path(path)) {
        return get_attribute_id(path);
    } else {
        throw std::invalid_argument("Invalid path " + path);
    }
}

constexpr const char* get_etcd_service_attribute_string(
    const uh::cluster::etcd_service_attributes& param) {
    for (const auto& entry : string_by_service_attribute) {
        if (entry.first == param)
            return entry.second;
    }

    throw std::invalid_argument("invalid etcd parameter");
}

constexpr uh::cluster::etcd_service_attributes
get_etcd_service_attribute_enum(const std::string& param) {
    for (const auto& entry : string_by_service_attribute) {
        if (entry.second == param)
            return entry.first;
    }

    throw std::invalid_argument("invalid etcd parameter");
}

} // namespace uh::cluster
