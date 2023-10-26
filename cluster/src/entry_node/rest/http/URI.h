#pragma once

#include <string>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <map>

namespace uh::cluster::rest::http
{

    namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
    namespace net = boost::asio;
    using tcp_stream = typename boost::beast::tcp_stream::rebind_executor<
            net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

    class URI
    {
    public:
        explicit URI(const http::request_parser<http::empty_body>&);
        ~URI() = default;

        [[nodiscard]] std::string get_bucket_id() const;
        [[nodiscard]] std::string get_object_key() const;
        [[nodiscard]] std::string get_query_string_value(const std::string& key) const;

    private:
        void extract_and_set_bucket_id_and_object_key();
        void extract_and_set_query_parameters();
        void extract_and_set_query_string();

        const http::request_parser<http::empty_body>& m_req;
        std::string m_target_string;
        std::string m_bucket_id;
        std::string m_object_key;
        std::string m_query_string;
        std::map<std::string, std::string> m_query_parameters;
    };

} // uh::cluster::rest::http
