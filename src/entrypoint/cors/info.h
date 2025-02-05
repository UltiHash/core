#pragma once

#include <entrypoint/http/raw_request.h>
#include <set>
#include <string>

namespace uh::cluster::ep::cors {

struct info {
    std::set<std::string> allowed_origins;
    std::set<http::verb> allowed_methods;
    std::set<std::string> allowed_headers;
    std::optional<std::string> exposed_headers;
    std::optional<unsigned> max_age_seconds;
};

} // namespace uh::cluster::ep::cors
