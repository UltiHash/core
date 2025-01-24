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

payg check_payg_license(std::string_view license, bool skip_verify = false);

void broadcast(etcd_manager& etcd, const payg& license);

} // namespace uh::cluster
