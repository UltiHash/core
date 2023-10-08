#include "entry_node/rest/utils/string/string_utils.h"
#include "http_request.h"

namespace uh::cluster::rest::http
{

    [[nodiscard]] std::map<std::string, std::string> http_request::get_headers() const
    {
        std::map<std::string, std::string> headers_map;

        for (const auto& pair : m_req.get() )
        {
            headers_map[rest::utils::string_utils::to_lower( std::string { pair.name_string().data(), pair.name_string().length() }.c_str() )] = pair.value();
        }

        return headers_map;
    }

    coro<void> http_request::read_body(uh::cluster::rest::http::tcp_stream &stream, boost::beast::flat_buffer &buffer)
    {
        if (m_req.get().has_content_length() &&
            m_req.content_length().value() != 0)
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
        }
        else
        {
            throw std::runtime_error("please specify the content length on requests as other methods without content length are currently not supported");
        }
    }

} // namespace uh::cluster::rest::http
