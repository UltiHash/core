#pragma once

#include "http_request.h"
#include "http_response.h"
#include "entrypoint/state/server_state.h"

namespace uh::cluster::entry {

    class put_object {
    public:

        bool can_handle(const http_request& req) const {
            if (req.get_method() == method::put) {

                const auto& uri = req.get_URI();
                if (!uri.get_bucket_id().empty() && !uri.get_object_key().empty() && uri.get_query_parameters().empty()) {
                    return true;
                }
            }

            return false;
        }

        http_response handle(const http_request& req) {
            return {};
        }

    private:
    };

} // uh::cluster::entrypoint namespace
