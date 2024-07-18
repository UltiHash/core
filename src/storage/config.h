#ifndef CORE_STORAGE_CONFIG_H
#define CORE_STORAGE_CONFIG_H

#include "common/network/server.h"
#include "common/types/big_int.h"
#include "data_store.h"
#include <filesystem>

namespace uh::cluster {

struct storage_config {
    server_config server = {
        .threads = 4,
        .port = 9200,
        .bind_address = "0.0.0.0",
    };

    int data_store_count = 1;
    data_store_config data_store = {
        .working_dir = "/var/lib/uh/storage",
        .file_size = 1 * GIBI_BYTE,
        .max_data_store_size = 1ul * PEBI_BYTE,
        .page_size = DEFAULT_PAGE_SIZE,
    };
};

} // namespace uh::cluster

#endif
