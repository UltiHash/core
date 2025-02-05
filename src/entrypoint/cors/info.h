#pragma once

#include <entrypoint/http/raw_request.h>
#include <set>
#include <string>

namespace uh::cluster::ep::cors {

struct info {
    std::string allowed_origin;

    std::set<http::verb> allowed_methods;
};

} // namespace uh::cluster::ep::cors
