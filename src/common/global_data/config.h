
#ifndef UH_CLUSTER_CONFIG_H
#define UH_CLUSTER_CONFIG_H

#include <cstdint>

namespace uh::cluster {

struct global_data_view_config {
    std::size_t storage_service_connection_count = 16;
    std::size_t read_cache_capacity_l2 = 4000ul;
};
} // namespace uh::cluster
#endif // UH_CLUSTER_CONFIG_H
