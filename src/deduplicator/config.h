#ifndef CORE_DEDUPLICATOR_CONFIG_H
#define CORE_DEDUPLICATOR_CONFIG_H

#include "common/global_data/config.h"
#include "common/network/server.h"
#include "storage/config.h"

#include <filesystem>

namespace uh::cluster {

constexpr std::size_t PREFIX_SIZE = 16;

struct deduplicator_config {
    server_config server = {
        .threads = 4,
        .port = 9300,
        .bind_address = "0.0.0.0",
    };

    global_data_view_config global_data_view;
    std::filesystem::path working_dir = "/var/lib/uh/deduplicator";
    std::size_t min_fragment_size = 32ul;
    std::size_t max_fragment_size = DEFAULT_PAGE_SIZE;
    std::size_t dedupe_worker_minimum_data_size = 128 * KIBI_BYTE;
    std::size_t worker_thread_count = 16ul;
    std::size_t fragment_buffer_size = 16ul * MEBI_BYTE;
    std::size_t set_capacity = 1000000;
    std::optional<storage_config> m_attached_storage;
};

} // namespace uh::cluster

#endif
