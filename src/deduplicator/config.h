#ifndef CORE_DEDUPLICATOR_CONFIG_H
#define CORE_DEDUPLICATOR_CONFIG_H

#include "common/global_data/global_data_view.h"
#include "common/network/server.h"

#include <filesystem>

namespace uh::cluster {

struct deduplicator_config {
    server_config server = {
        .threads = 4,
        .port = 9300,
        .bind_address = "0.0.0.0",
    };

    global_data_view_config global_data_view;
    std::filesystem::path working_dir = "/var/lib/uh/deduplicator";
    std::size_t min_fragment_size = 32ul;
    std::size_t max_fragment_size = 8 * KILO_BYTE;
    std::size_t dedupe_worker_minimum_data_size = 128 * KILO_BYTE;
    std::size_t worker_thread_count = 32ul;
};

} // namespace uh::cluster

#endif
