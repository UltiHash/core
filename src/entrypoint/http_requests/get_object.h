#pragma once

#include "http_request.h"
#include "http_response.h"

namespace uh::cluster::entry {

    class get_object {
    public:
        get_object() = default;

        static bool can_handle(const http_request& req) {
            return true;
        }

        coro <http_response> handle(const http_request& req) {
        }

    private:
    };

} // uh::cluster::entry
