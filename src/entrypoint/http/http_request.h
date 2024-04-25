#ifndef ENTRYPOINT_HTTP_HTTP_REQUEST_H
#define ENTRYPOINT_HTTP_HTTP_REQUEST_H

#include "uri.h"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <span>

namespace uh::cluster {
namespace http = boost::beast::http; // from <boost/beast/http.hpp>
template <typename T> using coro = boost::asio::awaitable<T>; // for coroutine

class transport_decoder {
public:
    ~transport_decoder() = default;
    virtual coro<std::size_t> read(std::span<char> dest) = 0;
};

class http_request {
public:
    static coro<std::unique_ptr<http_request>>
    create(boost::asio::ip::tcp::socket& s);

    [[nodiscard]] const uri& get_uri() const;

    [[nodiscard]] method get_method() const;

    coro<std::size_t> read_body(std::span<char> buffer);

    coro<void> respond(const http::response<http::string_body>& resp);

    [[nodiscard]] boost::asio::ip::tcp::socket& socket() const {
        return m_stream;
    }

    /** Payload that was read while reading the request headers.
     */
    std::size_t content_length() const {
        return std::stoul(m_req.at("Content-Length"));
    }

    bool keep_alive() const { return m_req.keep_alive(); }

private:
    friend std::ostream& operator<<(std::ostream& out, const http_request& req);

    http_request(boost::asio::ip::tcp::socket& stream,
                 http::request_parser<http::empty_body>::value_type&& req,
                 boost::beast::flat_buffer&& initial);

    boost::asio::ip::tcp::socket& m_stream;
    http::request_parser<http::empty_body>::value_type m_req;
    std::unique_ptr<transport_decoder> m_decoder;

    uri m_uri;
};

std::ostream& operator<<(std::ostream& out, const http_request& req);

} // namespace uh::cluster

#endif
