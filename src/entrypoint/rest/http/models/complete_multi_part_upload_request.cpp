#include "entrypoint/rest/utils/parser/xml_parser.h"
#include "custom_error_response_exception.h"
#include "complete_multi_part_upload_request.h"

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
        const auto up_info = m_server_state.m_uploads.get_upload_info(m_upload_id);
        if (up_info == nullptr) {
            throw custom_error_response_exception(http::status::not_found, error::type::no_such_upload);
        }

        for (const auto& objectNode : object_nodes_set)
        {
            auto part_num = std::stoi(objectNode.node().child("PartNumber").child_value());
            auto etag = objectNode.node().child("ETag").child_value();

            if (part_num != part_counter) {
                throw custom_error_response_exception(http::status::bad_request, error::type::invalid_part_oder);
            }

            if (up_info->part_sizes.at(part_num) < MAXIMUM_CHUNK_SIZE and part_num != up_info->part_sizes.size() - 1) {
                throw custom_error_response_exception(http::status::bad_request, error::type::entity_too_small);
            }

            if (up_info->etags.at(part_num) != etag ) {
                throw custom_error_response_exception(http::status::bad_request, error::type::invalid_part);
            }

            part_counter ++;
        }
    }

    void complete_multi_part_upload_request::validate_request_specific_criteria()
    {
        validate_request();
    }

} // uh::cluster::rest::http::model
