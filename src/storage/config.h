#pragma once

#include "interfaces/data_store.h"

#include <common/network/server.h>
#include <storage/global/config.h>

namespace uh::cluster {

struct storage_config {
    server_config server = {
        .threads = 4,
        .port = 9200,
        .bind_address = "0.0.0.0",
    };

    global_data_view_config global_data_view;

    std::filesystem::path working_directory;
    data_store_config data_store = {
        .max_file_size = 1 * GIBI_BYTE,
        .max_data_store_size = 1ul * PEBI_BYTE,
        .page_size = DEFAULT_PAGE_SIZE,
    };
    std::size_t instance_id = 0;
    std::size_t group_id = 0;
};

} // namespace uh::cluster
