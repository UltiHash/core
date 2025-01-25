#pragma once

#include <common/etcd/utils.h>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

namespace uh::cluster {

struct payg {
    enum type { FREEMIUM, PREMIUM };

    std::string customer_id;
    enum type license_type;
    std::size_t storage_cap;
    struct ec {
        bool enabled;
        std::size_t max_group_size;
    } ec;
    struct replication {
        bool enabled;
        std::size_t max_replicas;
    } replication;
};

class payg_handler {
public:
    explicit payg_handler(std::string_view json_str, bool skip_verify = false);
    auto get() const { return m_payg; }
    auto to_string() const { return m_compact_json; }

private:
    payg m_payg;
    std::string m_compact_json;
};

payg check_payg_license(std::string_view license, bool skip_verify = false);

} // namespace uh::cluster
