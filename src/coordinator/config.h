#pragma once

#include <common/db/config.h>
#include <common/license/backend_client.h>
#include <common/license/license.h>
#include <storage/group/config.h>

namespace uh::cluster {

struct coordinator_config {
    size_t thread_count = 1;
    uh::cluster::license license;
    // TODO: Initialize them using {}.
    std::vector<storage::group_config> storage_groups{{}};
    default_backend_client::config backend_config;
    db::config database_config;
};

} // end namespace uh::cluster
