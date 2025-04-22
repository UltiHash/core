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
    virtual const char* basename() const = 0;

    key_t(std::string&& prefix)
        : m_prefix{std::move(prefix)} {}

    operator std::string() const { return m_prefix + "/" + basename(); }

private:
    std::string m_prefix;
};

struct storage_states_t : public key_t {
    struct next_t : public key_t {
        std::string m_basename;
        template <std::integral T>
        next_t(std::string&& prefix, T index)
            : key_t{std::move(prefix)},
              m_basename{std::to_string(index)} {}
        const char* basename() const { return m_basename.c_str(); }
    };
    template <std::integral T> auto operator[](T index) {
        return next_t{*this, index};
    }
    storage_states_t(std::string&& prefix)
        : key_t{std::move(prefix)} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "storage_states";
};

struct group_initialized_t : public key_t {
    group_initialized_t(std::string&& prefix)
        : key_t{std::move(prefix)} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "group_initialized";
};

struct internals_t : public key_t {
    struct next_t : public key_t {
        std::string m_basename;
        storage_states_t storage_states;
        group_initialized_t group_initialized;
        template <std::integral T>
        next_t(std::string&& prefix, T index)
            : key_t{std::move(prefix)},
              m_basename{std::to_string(index)},
              storage_states{*this},
              group_initialized{*this} {}
        const char* basename() const { return m_basename.c_str(); }
    };
    template <std::integral T> auto operator[](T index) {
        return next_t{*this, index};
    }
    internals_t(std::string&& prefix)
        : key_t{std::move(prefix)} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "internals";
};

struct group_state_t : public key_t {
    group_state_t(std::string&& prefix)
        : key_t{std::move(prefix)} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "group_state";
};

struct storage_hostports_t : public key_t {
    struct next_t : public key_t {
        std::string m_basename;
        template <std::integral T>
        next_t(std::string&& prefix, T index)
            : key_t{std::move(prefix)},
              m_basename{std::to_string(index)} {}
        const char* basename() const { return m_basename.c_str(); }
    };
    template <std::integral T> auto operator[](T index) {
        return next_t{*this, index};
    }
    storage_hostports_t(std::string&& prefix)
        : key_t{std::move(prefix)} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "storage_hostports";
};

struct externals_t : public key_t {
    struct next_t : public key_t {
        std::string m_basename;
        group_state_t group_state;
        storage_hostports_t storage_hostports;
        template <std::integral T>
        next_t(std::string&& prefix, T index)
            : key_t{std::move(prefix)},
              m_basename{std::to_string(index)},
              group_state{*this},
              storage_hostports{*this} {}
        const char* basename() const { return m_basename.c_str(); }
    };
    template <std::integral T> auto operator[](T index) {
        return next_t{*this, index};
    }
    externals_t(std::string&& prefix)
        : key_t{std::move(prefix)} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "externals";
};

struct group_configs_t : public key_t {
    struct next_t : public key_t {
        std::string m_basename;
        template <std::integral T>
        next_t(std::string&& prefix, T index)
            : key_t{std::move(prefix)},
              m_basename{std::to_string(index)} {}
        const char* basename() const { return m_basename.c_str(); }
    };
    template <std::integral T> auto operator[](T index) {
        return next_t{*this, index};
    }
    group_configs_t(std::string&& prefix)
        : key_t{std::move(prefix)} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "group_configs";
};

struct storage_assignments_t : public key_t {
    struct next_t : public key_t {
        std::string m_basename;
        template <std::integral T>
        next_t(std::string&& prefix, T index)
            : key_t{std::move(prefix)},
              m_basename{std::to_string(index)} {}
        const char* basename() const { return m_basename.c_str(); }
    };
    template <std::integral T> auto operator[](T index) {
        return next_t{*this, index};
    }
    storage_assignments_t(std::string&& prefix)
        : key_t{std::move(prefix)} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "storage_assignments";
};

struct storage_group_t : public key_t {
    internals_t internals;
    externals_t externals;
    group_configs_t group_configs;
    storage_assignments_t storage_assignments;

    storage_group_t(std::string&& prefix)
        : key_t{std::move(prefix)},
          internals{*this},
          externals{*this},
          group_configs{*this},
          storage_assignments{*this} {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "storage_group";
};

struct uh_t : public key_t {
    storage_group_t storage_group;

    uh_t(std::string&& prefix = {})
        : key_t{std::move(prefix)},
          storage_group(*this) {}
    const char* basename() const override { return m_basename; }

private:
    inline static constexpr const char* m_basename = "uh";
};

inline uh_t root{};

} // namespace ns

} // namespace uh::cluster
