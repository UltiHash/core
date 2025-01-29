#pragma once

#include <common/license/backend_client.h>

namespace uh::cluster {

struct coordinator_config {
    size_t thread_count = 1;
    default_backend_client::config backend_config;
};

} // end namespace uh::cluster
