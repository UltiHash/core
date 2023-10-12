#include <iostream>
#include "s3_parser.h"
#include <entry_node/rest/http/models/put_object_request.h>
#include <entry_node/rest/http/models/get_object_request.h>
#include <entry_node/rest/http/models/create_bucket_request.h>
#include <entry_node/rest/http/models/list_buckets.h>
#include <entry_node/rest/http/models/init_multi_part_upload_request.h>
#include <entry_node/rest/http/models/multi_part_upload_request.h>
#include <entry_node/rest/http/models/complete_multi_part_upload_request.h>
#include <entry_node/rest/http/models/abort_multi_part_upload_request.h>
#include <regex>

namespace uh::cluster::rest::utils::parser {

//------------------------------------------------------------------------------

    s3_parser::s3_parser
    (const http::request_parser<http::empty_body>& recv_req,
     rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::string>>>& uomap)
     : m_recv_req(recv_req), m_uomap_multipart(uomap)
    {}

    std::unique_ptr<rest::http::http_request>
    s3_parser::parse() const
    {
        if (m_recv_req.get().base().version() != 11)
        {
            throw std::runtime_error("bad http version. support exists only for HTTP 1.1.\n");
        }

        auto target = m_recv_req.get().base().target();
        auto method = m_recv_req.get().base().method();

        std::regex pattern("^\\/\\w+$");
        std::string string_to_reg = target;

        switch (method)
        {
            case boost::beast::http::verb::post:
                if (target.ends_with("?uploads"))
                {
                    // mechanism for creating upload id, does this mechanism create same upload id for same POST request occurring twice?

                    m_uomap_multipart.emplace("first-upload", std::make_shared<utils::ts_map<uint16_t, std::string>>());

                    return std::make_unique<rest::http::model::init_multi_part_upload_request>(m_recv_req);
                }
                else if (target.find("?uploadId=") != std::string::npos)
                {
                    auto upload_id = std::string(target.substr(target.find("uploadId=") + 9));

                    return std::make_unique<rest::http::model::complete_multi_part_upload_request>(m_recv_req, m_uomap_multipart, upload_id);
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }
            case boost::beast::http::verb::put:
                if (std::regex_match(string_to_reg, pattern))
                {
                    return std::make_unique<rest::http::model::create_bucket_request>(m_recv_req);
                }
                else if (!target.empty() && (target.find('?') == std::string::npos))
                {
                    return std::make_unique<rest::http::model::put_object_request>(m_recv_req);
                }
                else if (target.find("?partNumber=") && target.find("&uploadId="))
                {

                    auto upload_id = std::string(target.substr(target.find("uploadId=") + 9, target.find("&partNumber=") - target.find("uploadId=") - 9 ));
                    auto part_number = std::stoi(std::string(target.substr(target.find("partNumber=") + 11)));

                    auto iterator = m_uomap_multipart.find(upload_id);
                    if (iterator == m_uomap_multipart.end())
                        throw std::runtime_error("Invalid Upload ID");

                    return std::make_unique<rest::http::model::multi_part_upload_request>(m_recv_req, *iterator->second, part_number);
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }
            case boost::beast::http::verb::get:
                if (target == "/")
                {
                    return std::make_unique<rest::http::model::list_buckets>(m_recv_req);
                }
                else if (!target.empty() && (target.find('?') == std::string::npos))
                {
                    return std::make_unique<rest::http::model::get_object_request>(m_recv_req);
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }
            case boost::beast::http::verb::delete_:
                if (target.find("?uploadId="))
                {
                    auto upload_id = std::string(target.substr(target.find("uploadId=") + 9));
                    if (upload_id.empty())
                        throw std::runtime_error("No upload ID given!");

                    return std::make_unique<rest::http::model::abort_multi_part_upload_request>(m_recv_req, m_uomap_multipart, upload_id);
                }
            default:
                throw std::runtime_error("bad http verb.");
        }

    }

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser
