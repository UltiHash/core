#ifndef ENTRYPOINT_HTTP_HTTP_REQUEST_H
#define ENTRYPOINT_HTTP_HTTP_REQUEST_H

#include <map>
#include <boost/beast/http.hpp>
#include <boost/beast/core.hpp>
#include <boost/asio.hpp>
#include "URI.h"
#include "entrypoint/utils/md5.h"

namespace uh::cluster
{
    namespace http = boost::beast::http;        // from <boost/beast/http.hpp>
    template <typename T>
    using coro =  boost::asio::awaitable <T>;   // for coroutine


    class http_request
    {
    public:

        http_request(const http::request_parser<http::empty_body>& req,
                      boost::asio::ip::tcp::socket& stream,
                      boost::beast::flat_buffer& buffer);

        const URI& get_URI() const;

        const std::string& get_body() const;

        std::size_t get_body_size() const;

        method get_method() const;

        coro<void> read_body();

    private:
        const http::request_parser<http::empty_body>& m_req;
        boost::asio::ip::tcp::socket& m_stream;
        boost::beast::flat_buffer& m_buffer;

        URI m_uri;
        std::string m_etag {};
        std::string m_body {};
    };

} // uh::cluster::entry

#endif
