#pragma once

#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>

namespace uh::cluster::rest::http::model
{

    namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
    namespace net = boost::asio;
    using tcp_stream = typename boost::beast::tcp_stream::rebind_executor<
            net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

    class error_response
    {
    public:
        error_response() = default;
        explicit error_response(http::response<http::string_body>);

        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object();

    private:
        http::response<http::string_body> m_error{http::status::internal_server_error, 11};
    };

} // namespace uh::cluster::rest::http::model
