#pragma once

namespace uh::cluster {

struct coordinator_config {
    size_t thread_count = 1;
    struct {
        std::string backend_domain;
        std::string customer_id;
        std::string access_token;
    } license;
};

} // end namespace uh::cluster
