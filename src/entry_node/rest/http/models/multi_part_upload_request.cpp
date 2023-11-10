#include "multi_part_upload_request.h"
#include "entry_node/rest/utils/hashing/hash.h"
namespace uh::cluster::rest::http::model
{

    multi_part_upload_request::multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                 rest::utils::ts_map<uint16_t, std::pair<std::string, std::string>>& container, uint16_t part_number,
                                                 std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_mpcontainer(container),
            m_part_number(part_number)
    {}

    std::map<std::string, std::string> multi_part_upload_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;

        return headers;
    }

    const std::string& multi_part_upload_request::get_body()
    {
        return m_mpcontainer.find(m_part_number)->second.second;
    }

    coro<void> multi_part_upload_request::read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer)
    {
        if (m_req.get().has_content_length())
        {
            std::size_t content_length = m_req.content_length().value();

            if (content_length != 0)
            {
                if (m_mpcontainer.find(m_part_number) == m_mpcontainer.end())
                {
                    m_mpcontainer[m_part_number].second.append(content_length, 0);

                    auto data_left = content_length - buffer.size();

                    // copy remaining bytes from flat buffer to body_buffer
                    boost::asio::buffer_copy(boost::asio::buffer(m_mpcontainer[m_part_number].second), buffer.data());
                    auto size_transferred = co_await boost::asio::async_read(stream.socket(), boost::asio::buffer(m_mpcontainer[m_part_number].second.data() + buffer.size(), data_left),
                                                                             boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);

                    if (size_transferred + buffer.size() != content_length)
                    {
                        throw std::runtime_error("error reading the http body");
                    }

                    // calculate and store etag for verification step in complete mp
                    rest::utils::hashing::MD5 md5;
                    m_mpcontainer[m_part_number].first = md5.calculateMD5(m_mpcontainer[m_part_number].second);
                }
            }
        }
        else
        {
            throw std::runtime_error("please specify the content length on requests as other methods without content length are currently not supported");
        }

        co_return;
    }

} // uh::cluster::rest::http::model
