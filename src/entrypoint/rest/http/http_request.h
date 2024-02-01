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

    enum class http_request_type
    {
        CREATE_BUCKET,
        GET_BUCKET,
        LIST_BUCKETS,
        DELETE_BUCKET,
        DELETE_OBJECTS,
        PUT_OBJECT,
        GET_OBJECT,
        DELETE_OBJECT,
        LIST_OBJECTS_V2,
        LIST_OBJECTS,
        GET_OBJECT_ATTRIBUTES,
        INIT_MULTIPART_UPLOAD,
        MULTIPART_UPLOAD,
        COMPLETE_MULTIPART_UPLOAD,
        ABORT_MULTIPART_UPLOAD,
        LIST_MULTI_PART_UPLOADS,
    };

    /**
     * Abstract class to represent an HTTP request.
     */
    class http_request
    {
    public:

        http_request(const http::request_parser<http::empty_body>& recv_request, std::unique_ptr<URI> uri);

        virtual ~http_request() = default;

        [[maybe_unused]] virtual void validate_request_specific_criteria() { };

        virtual coro<void> read_body(boost::asio::ip::tcp::socket& stream, boost::beast::flat_buffer& buffer);

        [[maybe_unused]] virtual inline void clear_body() { m_body.clear(); }

        [[nodiscard]] virtual inline std::size_t get_body_size() const { return m_body.size(); }

        [[nodiscard]] virtual http_request_type get_request_name() const = 0;

        [[nodiscard]] virtual std::map<std::string, std::string> get_request_specific_headers() const = 0;

        [[nodiscard]] virtual inline const std::string& get_body() const { return m_body; }

        [[nodiscard]] std::map<std::string, std::string> get_headers() const;

        [[nodiscard]] inline http_method get_method() const { return m_method; }

        [[nodiscard]] inline const URI& get_URI() const { return *m_uri; }

        [[nodiscard]] inline const std::string& get_eTag() const { return m_etag; }

    protected:
        const http::request_parser<http::empty_body>& m_req;
        http_method m_method;
        std::unique_ptr<URI> m_uri;
        std::string m_etag {};
        std::string m_body {};
    };

} // uh::cluster::rest::http
