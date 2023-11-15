#pragma once

#include "http_request.h"

namespace uh::cluster::rest::http
{

    /**
     * Abstract class for representing an Http Response.
     */
    class http_response
    {
    public:
        explicit http_response(const http_request&);
        http_response(const http_request&, http::response<http::string_body>);
        virtual ~http_response() = default;

        [[nodiscard]] inline const http_request& get_originating_request() const { return m_orig_req; }
        inline void set_error() { m_errorHasBeenSet = true; }

        [[nodiscard]] virtual const http::response<http::string_body>& get_response_specific_object() = 0;
        void set_error(http::response<http::string_body>);
        void set_body(std::string);
        void set_error_body(std::string body);

    protected:
        const http_request& m_orig_req;
        http::response<http::string_body> m_res;

        bool m_errorHasBeenSet = false;
        http::response<http::string_body> m_error;

        bool etagHasBeenSet = false;
        std::string m_etag;
    };

} // uh::cluster::rest::http
