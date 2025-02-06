#pragma once

#include <common/caches/timed_cache.h>
#include <entrypoint/directory.h>
#include <entrypoint/http/request.h>
#include <entrypoint/http/response.h>

#include "info.h"

#include <variant>

namespace uh::cluster::ep::cors {

struct result {
    // if set, use this response as HTTP response and do not run any other
    // commands.
    std::optional<http::response> response;

    // if set, add these header fields to the final response
    std::optional<std::map<std::string, std::string>> headers;
};

class module {
public:
    module(directory& dir);

    /**
     * Check the request using CORS configuration, return a cors::result.
     */
    coro<result> check(const http::request& request) const;

private:
    coro<std::shared_ptr<std::map<std::string, info>>>
    get_info(const std::string& bucket);

    directory& m_directory;
    timed_cache<std::string, std::shared_ptr<std::map<std::string, info>>>
        m_info_cache;
};

} // namespace uh::cluster::ep::cors
