#ifndef ENTRYPOINT_HTTP_HTTP_REQUEST_H
#define ENTRYPOINT_HTTP_HTTP_REQUEST_H

#include "common/utils/md5.h"
#include "uri.h"
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <map>

namespace uh::cluster {
namespace http = boost::beast::http; // from <boost/beast/http.hpp>
template <typename T> using coro = boost::asio::awaitable<T>; // for coroutine

class http_request {
public:

    [[nodiscard]] const uri& get_uri() const;

    [[nodiscard]] const std::string& get_body() const;

    [[nodiscard]] std::size_t get_body_size() const;

    [[nodiscard]] method get_method() const;

    coro<void> read_body();

    coro<void> respond (const http::response<http::string_body>& resp);

    [[nodiscard]] boost::asio::ip::tcp::socket& socket() const { return m_stream; }

    bool keep_alive() const { return m_req.keep_alive(); }

private:
    friend coro<std::unique_ptr<http_request>>
    read_request(boost::asio::ip::tcp::socket&);
    friend std::ostream& operator<<(std::ostream& out, const http_request& req);

    http_request(boost::asio::ip::tcp::socket& stream);

    boost::asio::ip::tcp::socket& m_stream;
    http::request_parser<http::empty_body> m_req;
    boost::beast::flat_buffer m_buffer;

    uri m_uri;
    std::string m_body{};
};

std::ostream& operator<<(std::ostream& out, const http_request& req);

coro<std::unique_ptr<http_request>>
read_request(boost::asio::ip::tcp::socket& s);

} // namespace uh::cluster

#endif
