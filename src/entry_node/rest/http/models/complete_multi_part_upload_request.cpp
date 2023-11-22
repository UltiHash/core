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
    {
        m_parts_container_ptr = m_internal_server_state.get_multipart_container().get_value(m_upload_id);

        // grab a hold of the parts map
        if (m_parts_container_ptr == nullptr)
        {
            throw custom_error_response_exception(http::status::not_found, error::type::no_such_upload);
        }
    }

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

            auto part_iterator = m_parts_container_ptr->find(part_num);
            if ( part_iterator == m_parts_container_ptr->end() || part_iterator->second.first != etag  )
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
        auto part_container_size = m_parts_container_ptr->size();
        for (const auto& part : *m_parts_container_ptr)
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
            for (const auto& part : *m_parts_container_ptr)
                m_completed_body += part.second.second;
        }

        return m_completed_body;
    }

    std::size_t complete_multi_part_upload_request::get_body_size() const
    {
        size_t body_size {};
        for (const auto& part : *m_parts_container_ptr)
            body_size += part.second.second.length();

        return body_size;
    }

    void complete_multi_part_upload_request::clear_body()
    {
        // TODO: does remove(upload_id) throw?
        m_internal_server_state.get_multipart_container().remove(m_upload_id);

        auto& bucket_multiparts = m_internal_server_state.get_bucket_multiparts();
        auto keys_map_ptr = bucket_multiparts.get_value(m_bucket_name);
        if (keys_map_ptr == nullptr)
        {
            throw custom_error_response_exception(http::status::not_found, error::type::bucket_not_found);
        }

        auto vector_ptr = keys_map_ptr->get_value(m_object_name);
        if (vector_ptr == nullptr)
        {
            throw custom_error_response_exception(http::status::not_found, error::type::object_not_found);
        }


        vector_ptr->remove(m_upload_id);
        // if there are no upload ids in the object map , remove the object map
        if (vector_ptr->is_empty())
            keys_map_ptr->remove(m_object_name);

        // if there are no objects in bucket remove the whole bucket
        if (keys_map_ptr->is_empty())
        {
            bucket_multiparts.remove(m_bucket_name);
        }
    }

    void complete_multi_part_upload_request::validate_request_specific_criteria()
    {
        validate_request();
    }

} // uh::cluster::rest::http::model
