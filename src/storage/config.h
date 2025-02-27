#pragma once

#include "common/network/server.h"
#include "interfaces/data_store.h"

namespace uh::cluster {

struct storage_config {
    server_config server = {
        .threads = 4,
        .port = 9200,
        .bind_address = "0.0.0.0",
    };

    std::list<std::filesystem::path> data_store_roots;
    data_store_config data_store = {
        .max_file_size = 1 * GIBI_BYTE,
        .max_data_store_size = 1ul * PEBI_BYTE,
        .page_size = DEFAULT_PAGE_SIZE,
    };
};

} // namespace uh::cluster
