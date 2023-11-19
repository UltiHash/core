#include "complete_multi_part_upload_request.h"
#include "entry_node/rest/utils/parser/xml_parser.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    complete_multi_part_upload_request::complete_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                           utils::state& server_state,
                                                                           std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_internal_server_state(server_state),
            m_bucket_name(m_uri->get_bucket_id()),
            m_object_name(m_uri->get_object_key()),
            m_upload_id(m_uri->get_query_string_value("uploadId"))
    {}

    complete_multi_part_upload_request::~complete_multi_part_upload_request()
    {
        clear_body();
    }

    std::map<std::string, std::string> complete_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

    void complete_multi_part_upload_request::validate_request() const
    {
        auto& multipart_container = m_internal_server_state.get_multipart_container();

        // upload id should exist
        auto iterator = multipart_container.find(m_upload_id);
        if (iterator == multipart_container.end())
        {
            throw custom_error_response_exception(http::status::not_found, error::type::no_such_upload);
        }

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

            auto part_iterator = iterator->second->find(part_num);
            if ( part_iterator == iterator->second->end() || part_iterator->second.first != etag  )
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
        auto part_container_size = iterator->second->size();
        for (const auto& part : *iterator->second)
        {
            if (part.second.second.size() < 5*1024*1024 && part.first < part_container_size)
            {
                throw custom_error_response_exception(http::status::bad_request, error::type::entity_too_small);
            }
        }
    }

    const std::string& complete_multi_part_upload_request::get_body()
    {
        if (m_completed_body.empty())
        {
            auto& multipart_container = m_internal_server_state.get_multipart_container();

            auto iterator = multipart_container.find(m_upload_id);
            if (iterator == multipart_container.end())
                throw std::runtime_error("Invalid Upload ID");

            for (const auto& part : *iterator->second)
                m_completed_body += part.second.second;
        }

        return m_completed_body;
    }

    std::size_t complete_multi_part_upload_request::get_body_size() const
    {
        auto& multipart_container = m_internal_server_state.get_multipart_container();

        auto iterator = multipart_container.find(m_upload_id);
        if (iterator == multipart_container.end())
            throw std::runtime_error("Invalid Upload ID");

        size_t body_size {};
        for (const auto& part : *iterator->second)
            body_size += part.second.second.length();

        return body_size;
    }

    void complete_multi_part_upload_request::clear_body()
    {
        m_internal_server_state.get_multipart_container().remove(m_upload_id);

        auto& bucket_multiparts = m_internal_server_state.get_bucket_multiparts();
        auto vector_itr = bucket_multiparts.find(m_bucket_name)->second->find(m_object_name)->second;
        vector_itr->remove(m_upload_id);
        if (vector_itr->is_empty())
            bucket_multiparts.remove(m_bucket_name);
    }

    void complete_multi_part_upload_request::validate_request_specific_criteria()
    {
        validate_request();
    }

} // uh::cluster::rest::http::model
