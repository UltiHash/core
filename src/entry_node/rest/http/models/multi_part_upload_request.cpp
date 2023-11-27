#include "multi_part_upload_request.h"
#include "entry_node/rest/utils/hashing/hash.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    multi_part_upload_request::multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                         utils::state& server_state,
                                                 std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_internal_server_state(server_state),
            m_part_number(std::stoi(m_uri->get_query_parameters().at("partNumber"))),
            m_upload_id(m_uri->get_query_parameters().at("uploadId"))
    {
        m_parts_container_ptr = m_internal_server_state.get_multipart_container().get_value(m_upload_id);

        // grab a hold of the parts map
        if (m_parts_container_ptr == nullptr)
        {
            throw custom_error_response_exception(http::status::not_found, error::type::no_such_upload);
        }
    }

    std::map<std::string, std::string> multi_part_upload_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;

        return headers;
    }

    const std::string& multi_part_upload_request::get_body()
    {
        return (*m_parts_container_ptr).find(m_part_number)->second.second;
    }

    [[nodiscard]] std::size_t multi_part_upload_request::get_body_size() const
    {
        return (*m_parts_container_ptr).find(m_part_number)->second.second.size();
    }

    coro<void> multi_part_upload_request::read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer)
    {
        if (m_req.get().has_content_length())
        {
            std::size_t content_length = m_req.content_length().value();

            if (content_length != 0)
            {
                if (m_parts_container_ptr->find(m_part_number) == m_parts_container_ptr->end())
                {
                    (*m_parts_container_ptr)[m_part_number].second.append(content_length, 0);

                    auto data_left = content_length - buffer.size();

                    // copy remaining bytes from flat buffer to body_buffer
                    boost::asio::buffer_copy(boost::asio::buffer((*m_parts_container_ptr)[m_part_number].second), buffer.data());
                    auto size_transferred = co_await boost::asio::async_read(stream.socket(), boost::asio::buffer((*m_parts_container_ptr)[m_part_number].second.data() + buffer.size(), data_left),
                                                                             boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);

                    if (size_transferred + buffer.size() != content_length)
                    {
                        throw std::runtime_error("error reading the http body");
                    }

                    // calculate and store etag for verification step in complete mp
                    rest::utils::hashing::MD5 md5;
                    (*m_parts_container_ptr)[m_part_number].first = md5.calculateMD5((*m_parts_container_ptr)[m_part_number].second);
                }
            }
        }
        else
        {
            throw custom_error_response_exception(boost::beast::http::status::no_content);
        }

        co_return;
    }

} // uh::cluster::rest::http::model
