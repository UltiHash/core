#ifndef CORE_DIRECTORY_CONFIG_H
#define CORE_DIRECTORY_CONFIG_H

#include "common/global_data/config.h"
#include "common/network/server.h"
#include "common/types/big_int.h"
#include "common/types/common_types.h"
#include "storage/config.h"
#include <filesystem>

namespace uh::cluster {

struct bucket_config {
    size_t min_file_size = 2 * GIBI_BYTE;
    size_t max_file_size = 64 * GIBI_BYTE;
    size_t max_storage_size = 256 * GIBI_BYTE;
    size_t max_chunk_size = std::numeric_limits<uint32_t>::max();
};

struct directory_store_config {
    std::filesystem::path working_dir = "/var/lib/uh/directory";
    bucket_config bucket_conf;
};

struct directory_config {
    server_config server = {
        .threads = 4,
        .port = 9400,
        .bind_address = "0.0.0.0",
    };
    directory_store_config directory_store_conf;
    std::size_t worker_thread_count = 8ul;
    std::size_t download_chunk_size = 64ul * MEBI_BYTE;
    uint128_t max_data_store_size = 4 * TEBI_BYTE;
    global_data_view_config global_data_view;
    std::optional<storage_config> m_attached_storage;
};

} // namespace uh::cluster

#endif
