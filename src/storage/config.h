#ifndef CORE_STORAGE_CONFIG_H
#define CORE_STORAGE_CONFIG_H

#include "common/network/server.h"
#include "common/types/big_int.h"
#include "data_store.h"
#include <filesystem>

namespace uh::cluster {

struct storage_config {
    server_config server = {
        .threads = 16,
        .port = 9200,
        .bind_address = "0.0.0.0",
    };

    data_store_config data_store = {
        .working_dir = "/var/lib/uh/storage",
        .min_file_size = 1 * GIGA_BYTE,
        .max_file_size = 4 * GIGA_BYTE,
        .max_data_store_size = 64 * GIGA_BYTE,
    };
};

} // namespace uh::cluster

#endif
