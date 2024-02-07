#pragma once

#include <optional>
#include "common/utils/log.h"
#include "http_requests/http_request.h"
#include "http_requests/http_response.h"

namespace uh::cluster::entry {

    class dispatcher {
    public:

        template <typename ...args>
        static http_response dispatch(const http_request& req, args... a) {
            runner r(req);

            (r << ... << a);

            if (r.m_res) {
                return *r.m_res;
            }

            throw std::runtime_error("request cannot be handled");
        }

    private:

        struct runner {
            explicit runner(const http_request& req) : m_req(req) {}

            template <typename request_type>
            runner& operator<<(request_type& req_type) {
                if (!m_res && req_type.can_handle(m_req)) {
                    m_res = req_type.handle(m_req);
                }

                return *this;
            }

            const http_request& m_req;
            std::optional<http_response> m_res;
        };

    };

} // uh::cluster::entrypoint  namespace
