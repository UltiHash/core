#pragma once

#include <entrypoint/http/raw_request.h>
#include <set>
#include <string>

namespace uh::cluster::ep::cors {

struct info {
    std::string origin;
    std::set<http::verb> methods;
    std::set<std::string> headers;
    std::optional<std::string> expose_headers;
    std::optional<unsigned> max_age_seconds;
};

} // namespace uh::cluster::ep::cors
