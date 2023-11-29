#include "multi_part_upload_request.h"
#include "entry_node/rest/utils/hashing/hash.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    multi_part_upload_request::multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                         utils::server_state& server_state,
                                                 std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_server_state(server_state),
            m_part_number(std::stoi(m_uri->get_query_parameters().at("partNumber"))),
            m_upload_id(m_uri->get_query_parameters().at("uploadId"))
    {

        /*
         * grab a hold of the parts container so that we don't get segmentation fault if
         * the given upload is aborted or completed
        */
        m_parts_container = m_server_state.m_uploads.get_parts_container(m_upload_id);
        if (m_parts_container == nullptr)
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
        auto single_part = m_parts_container->find(m_part_number);

        if (single_part == nullptr)
        {
            throw std::runtime_error("part not found");
        }

        return single_part->get_body();
    }

    [[nodiscard]] std::size_t multi_part_upload_request::get_body_size() const
    {
        auto single_part = m_parts_container->find(m_part_number);

        if (single_part == nullptr)
        {
            throw std::runtime_error("part not found");
        }

        return single_part->get_body().size();
    }

    coro<void> multi_part_upload_request::read_body(tcp_stream& stream, boost::beast::flat_buffer& buffer)
    {
        if (m_req.get().has_content_length())
        {
            std::size_t content_length = m_req.content_length().value();

            if (content_length != 0)
            {

                auto single_part = m_parts_container->find(m_part_number);

                if (single_part == nullptr)
                {
                    std::string read_body;
                    read_body.append(content_length, 0);

                    auto data_left = content_length - buffer.size();

                    // copy remaining bytes from flat buffer to body_buffer
                    boost::asio::buffer_copy(boost::asio::buffer(read_body), buffer.data());
                    auto size_transferred = co_await boost::asio::async_read(stream.socket(), boost::asio::buffer(read_body.data() + buffer.size(), data_left),
                                                                             boost::asio::transfer_exactly(data_left), boost::asio::use_awaitable);

                    if (size_transferred + buffer.size() != content_length)
                    {
                        throw std::runtime_error("error reading the http body");
                    }

                    m_parts_container->put_single_part(m_part_number, std::move(read_body));
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
