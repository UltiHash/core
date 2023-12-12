#include "complete_multi_part_upload_request.h"
#include "entry_node/rest/utils/parser/xml_parser.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    complete_multi_part_upload_request::
    complete_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                           utils::server_state& server_state,
                                                                           std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_server_state(server_state),
            m_bucket_name(m_uri->get_bucket_id()),
            m_object_name(m_uri->get_object_key()),
            m_upload_id(m_uri->get_query_string_value("uploadId"))
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

    complete_multi_part_upload_request::
    ~complete_multi_part_upload_request()
    {
        // THIS REMOVED ON ERROR ON COMPLETE MULTIPART REQUEST AS WELL.
        m_server_state.m_uploads.remove_upload(m_upload_id, m_bucket_name, m_object_name);
    }

    std::map<std::string, std::string> complete_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

    void complete_multi_part_upload_request::validate_request() const
    {
        rest::utils::parser::xml_parser parsed_xml;
        pugi::xpath_node_set object_nodes_set;

        try
        {
            if (!parsed_xml.parse(m_body))
                throw std::runtime_error("");

            object_nodes_set = parsed_xml.get_nodes_from_path("/CompleteMultipartUpload/Part");
            if (object_nodes_set.empty())
                throw std::runtime_error("");
        }
        catch(const std::exception& e)
        {
            throw custom_error_response_exception(http::status::bad_request, error::type::malformed_xml);
        }

        uint16_t part_counter = 1;
        for (const auto& objectNode : object_nodes_set)
        {
            auto part_num = std::stoi(objectNode.node().child("PartNumber").child_value());
            auto etag = objectNode.node().child("ETag").child_value();

            auto part = m_parts_container->find(part_num);
            if ( part == nullptr || part->etag != etag  )
            {
                throw custom_error_response_exception(http::status::bad_request, error::type::invalid_part);
            }

            if (part_num != part_counter)
            {
                throw custom_error_response_exception(http::status::bad_request, error::type::invalid_part_oder);
            }

            part_counter++;
        }

        // small entity
        auto parts_and_sizes = m_parts_container->get_parts();
        for (const auto& part : parts_and_sizes)
        {
            if (part.second.second < 5*1024*1024 && part.first < parts_and_sizes.size())
            {
                throw custom_error_response_exception(http::status::bad_request, error::type::entity_too_small);
            }
        }
    }

    const std::string& complete_multi_part_upload_request::get_body() const
    {
        if (m_completed_body.empty())
        {
            auto parts_and_sizes = m_parts_container->get_parts();
            for (const auto& part : parts_and_sizes)
            {
                m_completed_body += part.second.first;
            }
        }

        return m_completed_body;
    }

    std::size_t complete_multi_part_upload_request::get_body_size() const
    {
        std::size_t body_size {};

        auto parts_and_sizes = m_parts_container->get_parts();
        for (const auto& part : parts_and_sizes)
        {
            body_size += part.second.second;
        }

        return body_size;
    }

    void complete_multi_part_upload_request::validate_request_specific_criteria()
    {
        validate_request();
    }

} // uh::cluster::rest::http::model
