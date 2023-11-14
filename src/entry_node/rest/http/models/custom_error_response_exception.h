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


    class error {
    public:
        enum type {
            success = 0,
            fail = 1,
            unknown = 2,
            no_such_upload = 3,
            malformed_xml = 4,
            invalid_part = 5,
            invalid_part_oder = 6,
            entity_too_small = 7,
            bucket_not_found = 8,
            invalid_bucket_name = 9,
        };

        explicit error(type t = unknown);

        const std::pair<std::string, std::string>& message() const;
        uint32_t code() const;
        type operator*() const;

    private:
        type m_type;
    };

    class custom_error_response_exception : public std::exception
    {
    public:
        custom_error_response_exception() = default;
        explicit custom_error_response_exception(http::status, error::type = error::type::unknown);
        [[nodiscard]] const http::response<http::string_body>& get_response_specific_object();

        [[nodiscard]] const char* what() const noexcept override;

    private:
        http::response<http::string_body> m_res{http::status::bad_request, 11};
        error m_error;
    };

} // namespace uh::cluster::rest::http::model
