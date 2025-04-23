#pragma once

#include <magic_enum/magic_enum.hpp>

namespace uh::cluster::storage {

enum class storage_state : std::uint8_t { DOWN, NEW, ASSIGNED };

enum class group_state : std::uint8_t {
    UNDETERMINED,
    INITIALIZING,
    HEALTHY,
    DEGRADED,
    FAILED,
    RECOVERING
};

} // namespace uh::cluster::storage
