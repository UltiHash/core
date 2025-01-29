#pragma once

#include <common/license/backend_client.h>
#include <common/license/license.h>

namespace uh::cluster {

struct coordinator_config {
    size_t thread_count = 1;
    uh::cluster::license license;
    default_backend_client::config backend_config;
};

} // end namespace uh::cluster
