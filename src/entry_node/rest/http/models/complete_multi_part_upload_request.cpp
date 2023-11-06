#include "complete_multi_part_upload_request.h"
#include "entry_node/rest/utils/parser/xml_parser.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    complete_multi_part_upload_request::complete_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                           rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::string>>>& uo_container,
                                                                           std::string upload_id, std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_uomap_multipart(uo_container),
            m_upload_id(std::move(upload_id))
    {
    }

    complete_multi_part_upload_request::~complete_multi_part_upload_request()
    {
        clear_body();
    }

    std::map<std::string, std::string> complete_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

    void complete_multi_part_upload_request::parse_and_check_xml() const
    {

        // upload id should exist
        auto iterator = m_uomap_multipart.find(m_upload_id);
        if (iterator == m_uomap_multipart.end())
        {
            std::string m_body = "<Error>\n"
                                 "<Code>NoSuchUpload</Code>\n"
                                 "<Message>Upload id not found.</Message>\n"
                                 "</Error>";

            throw custom_error_response_exception(http::response<http::string_body>{http::status::not_found, 11}, std::move(m_body));
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
            std::string m_body = "<Error>\n"
                                 "<Code>MalformedXML</Code>\n"
                                 "<Message>XML is invalid.</Message>\n"
                                 "</Error>";

            throw custom_error_response_exception(http::response<http::string_body>{http::status::bad_request, 11}, std::move(m_body));
        }

        for (const auto& objectNode : object_nodes_set)
        {
            if ( iterator->second->find(std::stoi(objectNode.node().child("PartNumber").child_value())) == iterator->second->end() )
            {
                std::string m_body = "<Error>\n"
                                     "<Code>InvalidPart</Code>\n"
                                     "<Message>Part not found.</Message>\n"
                                     "</Error>";

                throw custom_error_response_exception(http::response<http::string_body>{http::status::bad_request, 11}, std::move(m_body));
            }
        }
    }

    const std::string& complete_multi_part_upload_request::get_body()
    {
        if (m_completed_body.empty())
        {
            auto iterator = m_uomap_multipart.find(m_upload_id);
            if (iterator == m_uomap_multipart.end())
                throw std::runtime_error("Invalid Upload ID");

            for (const auto& part : *iterator->second)
                m_completed_body += part.second;
        }

        return m_completed_body;
    }

    std::size_t complete_multi_part_upload_request::get_body_size() const
    {
        auto iterator = m_uomap_multipart.find(m_upload_id);
        if (iterator == m_uomap_multipart.end())
            throw std::runtime_error("Invalid Upload ID");

        size_t body_size {};
        for (const auto& part : *iterator->second)
            body_size += part.second.length();

        return body_size;
    }

    void complete_multi_part_upload_request::clear_body()
    {
        m_uomap_multipart.remove(m_upload_id);
    }

    void complete_multi_part_upload_request::handle_request_specific_criteria()
    {
        parse_and_check_xml();
    }

} // uh::cluster::rest::http::model
