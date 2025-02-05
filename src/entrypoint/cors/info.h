#pragma once

#include <string>

namespace uh::cluster::ep::cors {

struct info {
    std::string allowed_origin;

    bool allowed_delete = false;
    bool allowed_get = false;
    bool allowed_head = false;
    bool allowed_post = false;
    bool allowed_put = false;
};

} // namespace uh::cluster::ep::cors
