#pragma once

#include <cstdint>

namespace uh::cluster {

struct global_data_view_config {
    std::size_t storage_service_connection_count = 16;
    std::size_t read_cache_capacity_l2 = 400000ul;
    // TODO: remove below
    std::size_t ec_data_shards = 1;
    std::size_t ec_parity_shards = 0;
};
} // namespace uh::cluster
