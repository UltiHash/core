#pragma once

#include <map>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include "URI.h"

namespace uh::cluster::entry
{
    namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
    template <typename T>
    using coro =  boost::asio::awaitable <T>;   // for coroutine

    /**
     * Abstract class to represent an HTTP request.
     */
    class http_request
    {
    public:

        explicit http_request(const http::request_parser<http::empty_body>& req) :
        m_req(req),
        m_uri(req)
        {}

        inline const URI& get_URI() const {
            return m_uri;
        }

        inline const std::string& get_eTag() const {
            return m_etag;
        }

        inline std::string_view get_body() const {
            return m_body;
        }

        inline method get_method() const {
            return m_uri.get_method();
        }

    private:
        const http::request_parser<http::empty_body>& m_req;
        URI m_uri;
        std::string m_etag {};
        std::string m_body {};
    };

} // uh::cluster::rest::http
