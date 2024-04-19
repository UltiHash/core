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
      m_uri(m_req) {}

const uri& http_request::get_uri() const { return m_uri; }

const std::string& http_request::get_body() const { return m_body; }

std::size_t http_request::get_body_size() const { return m_body.size(); }

method http_request::get_method() const { return m_uri.get_method(); }

coro<void> http_request::read_body() {
    try {
        std::size_t length = content_length();
        if (length == 0) {
            co_return;
        }

        m_body.append(length, 0);

        auto data_left = length - m_buffer.size();

        // copy remaining bytes from flat buffer to body_buffer
        boost::asio::buffer_copy(boost::asio::buffer(m_body), m_buffer.data());
        auto size_transferred = co_await boost::asio::async_read(
            m_stream,
            boost::asio::buffer(m_body.data() + m_buffer.size(), data_left),
            boost::asio::transfer_exactly(data_left),
            boost::asio::use_awaitable);

        if (size_transferred + m_buffer.size() != length) {
            throw std::runtime_error("error reading the http body");
        }
    } catch (const std::out_of_range&) {
        throw std::runtime_error(
            "please specify the content length on requests as other methods "
            "without content length are currently not supported");
    }

    co_return;
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
