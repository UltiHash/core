#pragma once

#include <map>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include "http_types.h"
#include "URI.h"

namespace uh::cluster::rest::http
{
    namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
    namespace net = boost::asio;
    using tcp_stream = typename boost::beast::tcp_stream::rebind_executor<
            net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;
    template <typename T>
    using coro =  boost::asio::awaitable <T>;   // for coroutine

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

        explicit http_request(const http::request_parser<http::empty_body>& recv_request) :
        m_req(recv_request), m_method(get_http_method_from_name(recv_request.get().base().method()))
        {}

        virtual ~http_request() = default;

        [[nodiscard]] virtual const char * get_request_name() const = 0;

        [[nodiscard]] virtual std::map<std::string, std::string> get_request_specific_headers() const = 0;

        virtual coro<void> read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer) = 0;

        [[nodiscard]] inline std::string get_body() const { return m_body; }

        [[nodiscard]] std::map<std::string, std::string> get_headers() const;

        [[nodiscard]] inline http_method get_method() const { return m_method; }

    protected:
        const http::request_parser<http::empty_body>& m_req;
        http_method m_method;
        URI m_uri;
        std::string m_body {};
    };

} // uh::cluster::rest::http
