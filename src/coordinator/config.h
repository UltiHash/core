#pragma once

#include <common/db/config.h>
#include <common/license/backend_client.h>
#include <common/license/license.h>

namespace uh::cluster {

struct coordinator_config {
    size_t thread_count = 1;
    uh::cluster::license license;
    default_backend_client::config backend_config;
    db::config database_config;
    std::size_t ec_data_shards = 1;
    std::size_t ec_parity_shards = 0;
};

} // end namespace uh::cluster
