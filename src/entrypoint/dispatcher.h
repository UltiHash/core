#pragma once

#include <optional>
#include "common/utils/log.h"
#include "http_requests/http_request.h"
#include "http_requests/http_response.h"

namespace uh::cluster::entry {

    template <typename ...args>
    static coro <http_response> dispatch(const http_request& req, args&&... commands) {

        std::optional<http_response> response;

//        auto command_references = std::tie(commands...);

        if (response) {
            co_return *response;
        }

        throw std::runtime_error("request cannot be handled");
    };

} // uh::cluster::entrypoint  namespace
