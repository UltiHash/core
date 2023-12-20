#pragma once

#include <string>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include <map>
#include <boost/url/decode_view.hpp>
#include <boost/url/url.hpp>
#include "http_types.h"

namespace uh::cluster::rest::http
{

    namespace http = boost::beast::http;           // from <boost/beast/http.hpp>
    namespace net = boost::asio;
    using tcp_stream = typename boost::beast::tcp_stream::rebind_executor<
            net::use_awaitable_t<>::executor_with_default<net::any_io_executor>>::other;

    /**
    * Enum representing URI scheme.
    */
    enum class scheme
    {
        HTTP,
        HTTPS
    };

    const char* to_string(scheme scheme);
    scheme from_string(const char* name);

    class URI
    {
    public:
        explicit URI(const http::request_parser<http::empty_body>&);
        ~URI() = default;

        [[nodiscard]] const std::string& get_bucket_id() const;
        [[nodiscard]] const std::string& get_object_key() const;
        [[nodiscard]] bool query_string_exists(const std::string& key) const;
        [[nodiscard]] const std::string& get_query_string_value(const std::string& key) const;
        [[nodiscard]] const std::map<std::string, std::string>& get_query_parameters() const;
        [[nodiscard]] http_method get_http_method() const;

    private:
        void extract_and_set_bucket_id_and_object_key();
        void extract_and_set_query_parameters();

        const http::request_parser<http::empty_body>& m_req;
        scheme m_scheme = scheme::HTTP; // TODO: mechanism to determin whether HTTP or HTTPS
        http_method m_method;
        std::string m_bucket_id {};
        std::string m_object_key {};
        boost::urls::url m_url;
        std::map<std::string, std::string> m_query_parameters;
    };

} // uh::cluster::rest::http
