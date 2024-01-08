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
    }

    void complete_multi_part_upload_request::validate_request_specific_criteria()
    {
        validate_request();
    }

} // uh::cluster::rest::http::model
