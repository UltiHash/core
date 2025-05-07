#pragma once

#define NAMESPACE "uh"

#include <common/utils/common.h>

#include <array>
#include <filesystem>
#include <map>
#include <string>

namespace uh::cluster {

static constexpr const char* etcd_watchdog = "/" NAMESPACE "/watchdog/";

static constexpr const char* etcd_services_key_prefix =
    "/" NAMESPACE "/services/";

static constexpr const char* etcd_global_lock_key =
    "/" NAMESPACE "/config/class/cluster/lock";
static constexpr const char* etcd_current_id_prefix_key =
    "/" NAMESPACE "/config/class/cluster/current_id/";

static constexpr const char* etcd_storage_lock_key =
    "/" NAMESPACE "/config/class/storage/lock";
static constexpr const char* etcd_registered_storage_ids_prefix_key =
    "/" NAMESPACE "/config/class/storage/registered_ids/";

static constexpr const char* etcd_license_key = "/" NAMESPACE "/config/license";

enum class etcd_action : uint8_t {
    CREATE = 0,
    SET,
    DELETE,
    GET,
};

inline etcd_action get_etcd_action_enum(const std::string& action_str) {
    static const std::map<std::string, etcd_action> etcd_action = {
        {"create", etcd_action::CREATE},
        {"set", etcd_action::SET},
        {"delete", etcd_action::DELETE},
        {"get", etcd_action::GET},
    };

    if (const auto f = etcd_action.find(action_str); f != etcd_action.cend())
        return f->second;

    throw std::invalid_argument("invalid etcd action");
}

enum etcd_service_attributes {
    ENDPOINT_HOST,
    ENDPOINT_PORT,
};

constexpr std::array<
    std::pair<uh::cluster::etcd_service_attributes, const char*>, 2>
    string_by_service_attribute = {{
        {uh::cluster::ENDPOINT_HOST, "endpoint_host"},
        {uh::cluster::ENDPOINT_PORT, "endpoint_port"},
    }};

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

namespace ns {

struct key_t {
    key_t() = default;
    key_t(std::string&& basename, key_t* parent = nullptr) {
        m_basename = std::move(basename);
        if (parent) {
            m_key = parent->m_key + "/" + m_basename;
        } else {
            m_key = "/" + m_basename;
        }
    }
    ~key_t() = default;
    key_t(const key_t&) = default;
    key_t& operator=(const key_t&) = default;
    key_t(key_t&&) = default;
    key_t& operator=(key_t&&) = default;
    operator std::string() const { return m_key; }
    const std::string_view basename() const { return m_basename; }

private:
    std::string m_basename;
    std::string m_key;
};

struct subscriptable_key_t : public key_t {
    struct impl_t : public key_t {
        using key_t::key_t;
    };
    template <std::integral T> auto operator[](T index) {
        return impl_t{std::to_string(index), this};
    }
    using key_t::key_t;
};

struct temporaries_t : public key_t {
    struct impl_t : public key_t {
        subscriptable_key_t storage_offsets{"storage_offsets", this};
        using key_t::key_t;
    };
    template <std::integral T> auto operator[](T index) {
        return impl_t{std::to_string(index), this};
    }
    using key_t::key_t;
};

struct storage_groups_t : public key_t {
    subscriptable_key_t group_configs{"group_configs", this};
    subscriptable_key_t storage_assignments{"storage_assignments", this};
    temporaries_t temporaries{"temporaries", this};

    struct impl_t : public key_t {
        subscriptable_key_t storage_hostports{"storage_hostports", this};
        subscriptable_key_t storage_states{"storage_states", this};
        subscriptable_key_t storage_assignment_triggers{
            "storage_assignment_triggers", this};
        key_t group_initialized{"group_initialized", this};
        key_t group_state{"group_state", this};
        key_t leader{"leader", this};
        using key_t::key_t;
    };
    template <std::integral T> auto operator[](T index) {
        return impl_t{std::to_string(index), this};
    }
    using key_t::key_t;
};

struct service_t : public key_t {
    subscriptable_key_t hostports{"hostports", this};
    using key_t::key_t;
};

struct uh_t : public key_t {
    storage_groups_t storage_groups{"storage_groups", this};
    service_t deduplicator{"deduplicator", this};
    service_t entrypoint{"entrypoint", this};
    service_t coordinator{"coordinator", this};

    using key_t::key_t;
};

inline uh_t root{"uh"};

} // namespace ns

} // namespace uh::cluster
