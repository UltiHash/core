#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace uh::cluster::storage_group {

struct state {
    enum class group_state : std::uint8_t {
        UNDETERMINED,
        INITIALIZING,
        HEALTHY,
        DEGRADED,
        FAILED,
        RECOVERING,
        SIZE
    };
    enum class storage_state : std::uint8_t { NEW, ASSIGNED, DOWN, SIZE };

    enum group_state group { group_state::UNDETERMINED };
    std::vector<enum storage_state> storages;

    static state create(std::string_view str);
    std::string to_string() const;
};

} // namespace uh::cluster::storage_group
