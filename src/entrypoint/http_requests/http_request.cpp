#include "http_request.h"

namespace uh::cluster {

    http_request::http_request(const http::request_parser<http::empty_body>& req,
                 boost::asio::ip::tcp::socket& stream,
                 boost::beast::flat_buffer& buffer) :
            m_req(req),
            m_stream(stream),
            m_buffer(buffer),
            m_uri(req)
    {}

    const URI& http_request::get_URI() const {
        return m_uri;
    }

    const std::string& http_request::get_body() const {
        return m_body;
    }

    std::size_t http_request::get_body_size() const {
        return m_body.size();
    }

    method http_request::get_method() const {
        return m_uri.get_method();
    }

    coro<void> http_request::read_body() {
        if (m_req.get().has_content_length())
        {
            if (m_req.content_length().value() != 0)
            {
                std::size_t content_length = m_req.content_length().value();
                m_body.append(content_length, 0);

                auto data_left = content_length - m_buffer.size();

                // copy remaining bytes from flat buffer to body_buffer
                boost::asio::buffer_copy(boost::asio::buffer(m_body), m_buffer.data());
                auto size_transferred = co_await boost::asio::async_read(m_stream, boost::asio::buffer(m_body.data() + m_buffer.size(), data_left),
                                                                         boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);

                if (size_transferred + m_buffer.size() != content_length)
                {
                    throw std::runtime_error("error reading the http body");
                }

                m_etag = md5::calculateMD5(m_body);
            }
        }
        else
        {
            throw std::runtime_error("please specify the content length on requests as other methods without content length are currently not supported");
        }

        co_return;
    }

} // namespace uh::cluster