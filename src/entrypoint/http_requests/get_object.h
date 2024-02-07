#pragma once

#include "http_request.h"
#include "http_response.h"

namespace uh::cluster::entry {

    class get_object {
    public:
        get_object() = default;

        bool can_handle(const http_request& req) const {
            return true;
        }

        http_response handle(const http_request& req) {
            return {};
        }

    private:
    };

} // uh::cluster::entrypoint namespace
