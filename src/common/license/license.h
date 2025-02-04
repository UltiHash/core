#pragma once

#include <common/etcd/utils.h>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

namespace uh::cluster {

struct license {
    enum type { NONE, FREEMIUM, PREMIUM };

    std::string version;
    std::string customer_id;
    enum type license_type { NONE };
    std::size_t storage_cap_gib{0};

    operator bool() const { return is_valid(); }

    enum class verify : std::uint8_t { VERIFY, SKIP_VERIFY };

    static license create(std::string_view json_str,
                          verify option = verify::VERIFY);

    std::string to_string() const { return m_compact_json; };

private:
    bool is_valid() const { return license_type != type::NONE; }

    std::string m_compact_json;
};

} // namespace uh::cluster
