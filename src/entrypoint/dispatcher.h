#pragma once

#include <optional>
#include "common/utils/log.h"
#include "http_requests/http_request.h"
#include "http_requests/http_response.h"

namespace uh::cluster::entry {

    static coro<http_response> call(const http_request& req) {
        throw command_unknown_exception();
    }

    template <typename command, typename ... commands>
    static coro<http_response> call(http_request& req, command&& head, commands&& ... tail) {
        if (head.can_handle(req)) {
            return head.handle(req);
        }

        return call(req, tail...);
    }

    template <typename ...commands>
    static coro <http_response> dispatch(http_request& req, commands&&... a) {
         return call(req, a...);
    }

} // uh::cluster::entry  namespace
