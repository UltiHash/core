#pragma once

#include <common/db/config.h>
#include <common/license/backend_client.h>
#include <common/license/license.h>
#include <storage/group/config.h>

namespace uh::cluster {

struct coordinator_config {
    std::size_t num_threads = 2;

    uh::cluster::license license;
    storage::group_configs storage_groups;
    default_backend_client::config backend_config;
    db::config database_config;
};

} // end namespace uh::cluster
