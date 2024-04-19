#include "http_request.h"

namespace uh::cluster {

using namespace boost;

coro<std::unique_ptr<http_request>>
http_request::create(asio::ip::tcp::socket& s) {

    http::request_parser<http::empty_body> req;
    boost::beast::flat_buffer buffer;

    co_await beast::http::async_read_header(s, buffer, req,
                                            asio::use_awaitable);

    co_return std::unique_ptr<http_request>(
        new http_request(s, std::move(req.get()), std::move(buffer)));
}

http_request::http_request(
    boost::asio::ip::tcp::socket& stream,
    http::request_parser<http::empty_body>::value_type&& req,
    boost::beast::flat_buffer&& buffer)
    : m_stream(stream),
      m_req(std::move(req)),
      m_buffer(std::move(buffer)),
      m_uri(m_req),
      m_body_read(0ull) {}

const uri& http_request::get_uri() const { return m_uri; }

method http_request::get_method() const { return m_uri.get_method(); }

coro<std::size_t> http_request::read_body(std::span<char> buffer) {
    std::size_t offs = 0;

    if (m_buffer.size() > 0) {
        auto read = asio::buffer_copy(asio::buffer(buffer), m_buffer.data());
        m_buffer.clear();

        m_body_read += read;
        offs += read;
    }

    auto count = std::min(content_length() - m_body_read, buffer.size() - offs);

    auto read = co_await asio::async_read(
        m_stream, asio::buffer(buffer.data() + offs, count),
        asio::use_awaitable);
    m_body_read += read;
    co_return offs + read;
}

coro<void>
http_request::respond(const http::response<http::string_body>& resp) {
    co_await boost::beast::http::async_write(m_stream, resp,
                                             boost::asio::use_awaitable);
}

std::ostream& operator<<(std::ostream& out, const http_request& req) {
    out << req.m_req.base().method_string() << " " << req.m_req.base().target()
        << " ";

    std::string delim;
    for (const auto& field : req.m_req.base()) {
        out << delim << field.name_string() << ": " << field.value();
        delim = ", ";
    }

    return out;
}

} // namespace uh::cluster
