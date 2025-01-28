#pragma once

#include <common/etcd/utils.h>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

namespace uh::cluster {

struct payg_license {
    enum type { NONE, FREEMIUM, PREMIUM };

    std::string version;
    std::string customer_id;
    enum type license_type { NONE };
    std::size_t storage_cap{0};
    struct ec {
        bool enabled{false};
        std::size_t max_group_size;
    } ec;
    struct replication {
        bool enabled{false};
        std::size_t max_replicas;
    } replication;

    operator bool() const { return is_valid(); }

    enum class verify : std::uint8_t { VERIFY, SKIP_VERIFY };

    static payg_license create_from_json(std::string_view json_str,
                                         verify option = verify::VERIFY);

    std::string to_json_string() const { return m_compact_json; };

private:
    bool is_valid() const { return !version.empty(); }

    std::string m_compact_json;
};

} // namespace uh::cluster
