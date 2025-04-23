#pragma once

#include <common/etcd/registry/storage_registry.h>
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <string_view>
#include <vector>

namespace uh::cluster::storage::group {

enum class group_state : std::uint8_t {
    UNDETERMINED,
    INITIALIZING,
    HEALTHY,
    DEGRADED,
    FAILED,
    RECOVERING
};

struct state_context {
    enum group_state group { group_state::UNDETERMINED };
    std::vector<enum storage_state> storages;

    static state_context create(std::string_view str);
    std::string to_string() const;
};

} // namespace uh::cluster::storage::group
