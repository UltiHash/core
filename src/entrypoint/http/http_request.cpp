#include "http_request.h"

namespace uh::cluster {

using namespace boost;

http_request::http_request(asio::ip::tcp::socket& stream)
    : m_stream(stream),
      m_req(),
      m_buffer(),
      m_uri(m_req) {
    m_req.body_limit(std::numeric_limits<std::uint64_t>::max());
}

const uri& http_request::get_uri() const { return m_uri; }

const std::string& http_request::get_body() const { return m_body; }

std::size_t http_request::get_body_size() const { return m_body.size(); }

method http_request::get_method() const { return m_uri.get_method(); }

coro<void> http_request::read_body() {
    if (m_req.get().has_content_length()) {
        if (m_req.content_length().value() != 0) {
            std::size_t content_length = m_req.content_length().value();
            m_body.append(content_length, 0);

            auto data_left = content_length - m_buffer.size();

            // copy remaining bytes from flat buffer to body_buffer
            boost::asio::buffer_copy(boost::asio::buffer(m_body),
                                     m_buffer.data());
            auto size_transferred = co_await boost::asio::async_read(
                m_stream,
                boost::asio::buffer(m_body.data() + m_buffer.size(), data_left),
                boost::asio::transfer_exactly(data_left),
                boost::asio::use_awaitable);

            if (size_transferred + m_buffer.size() != content_length) {
                throw std::runtime_error("error reading the http body");
            }
        }
    } else {
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
    out << req.m_req.get().base().method_string() << " "
        << req.m_req.get().base().target() << " ";

    std::string delim;
    for (const auto& field : req.m_req.get().base()) {
        out << delim << field.name() << ": " << field.value();
        delim = ", ";
    }

    return out;
}

coro<std::unique_ptr<http_request>> read_request(asio::ip::tcp::socket& s) {
    auto rv = std::unique_ptr<http_request>(new http_request(s));

    co_await beast::http::async_read_header(s, rv->m_buffer, rv->m_req,
                                            asio::use_awaitable);

    rv->m_uri = uri(rv->m_req);

    co_return rv;
}

} // namespace uh::cluster
