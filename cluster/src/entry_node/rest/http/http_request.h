#pragma once

#include <map>
#include <boost/beast/http.hpp>
#include "http_types.h"

namespace uh::cluster::rest::http
{
    namespace http = boost::beast::http;           // from <boost/beast/http.hpp>

    /**
     * Enum to represent version of the http protocol to use
     */
    enum class http_version
    {
        HTTP_VERSION_NONE,
        HTTP_VERSION_1_0,
        HTTP_VERSION_1_1,
        HTTP_VERSION_2_0,
        HTTP_VERSION_2TLS,
        HTTP_VERSION_2_PRIOR_KNOWLEDGE,
        HTTP_VERSION_3,
        HTTP_VERSION_3ONLY,
    };

    /**
     * Enum representing URI scheme.
     */
    enum class scheme
    {
        HTTP,
        HTTPS
    };

    /**
     * Abstract class to represent an HTTP request.
     */
    class http_request
    {
    public:

        http_request(const http::request_parser<http::empty_body>& recv_request) :
        m_req(recv_request)
        {}

        virtual ~http_request() = default;

        virtual std::map<std::string, std::string> get_headers() const = 0;

        virtual const char * get_request_name() const = 0;

    private:
        http_method m_method;
        const http::request_parser<http::empty_body>& m_req;
    };

} // uh::cluster::rest::http
