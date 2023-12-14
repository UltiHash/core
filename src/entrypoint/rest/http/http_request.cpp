#include <iostream>
#include "../utils/string/string_utils.h"
#include "../utils/hashing/hash.h"

#include "http_request.h"

namespace uh::cluster::rest::http
{

    http_request::http_request(const http::request_parser<http::empty_body>& recv_request, std::unique_ptr<URI> uri) :
            m_req(recv_request), m_method(get_http_method_from_beast(recv_request.get().base().method())),
            m_uri(std::move(uri))
    {
    }

    [[nodiscard]] std::map<std::string, std::string> http_request::get_headers() const
    {
        std::map<std::string, std::string> headers_map;

        for (const auto& pair : m_req.get() )
        {
            headers_map[rest::utils::string_utils::to_lower( std::string { pair.name_string().data(), pair.name_string().length() }.c_str() )] = pair.value();
        }

        return headers_map;
    }

    coro<void> http_request::read_body(uh::cluster::rest::http::tcp_stream& stream, boost::beast::flat_buffer& buffer)
    {
        if (m_req.get().has_content_length())
        {
            if (m_req.content_length().value() != 0)
            {
                std::size_t content_length = m_req.content_length().value();
                m_body.append(content_length, 0);

                auto data_left = content_length - buffer.size();

                // copy remaining bytes from flat buffer to body_buffer
                boost::asio::buffer_copy(boost::asio::buffer(m_body), buffer.data());
                auto size_transferred = co_await boost::asio::async_read(stream.socket(), boost::asio::buffer(m_body.data() + buffer.size(), data_left),
                                                                         boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);

                if (size_transferred + buffer.size() != content_length)
                {
                    throw std::runtime_error("error reading the http body");
                }

                rest::utils::hashing::MD5 md5 {};
                m_etag = md5.calculateMD5(m_body);
            }
        }
        else
        {
            throw std::runtime_error("please specify the content length on requests as other methods without content length are currently not supported");
        }

        co_return;
    }

} // namespace uh::cluster::rest::http
