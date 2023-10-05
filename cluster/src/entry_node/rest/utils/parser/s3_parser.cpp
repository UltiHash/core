#include <iostream>
#include "s3_parser.h"
#include <entry_node/rest/http/models/put_object.h>
#include <entry_node/rest/http/models/init_multi_part_upload.h>
#include <memory>

namespace uh::cluster::rest::utils::parser {

//------------------------------------------------------------------------------

    s3_parser::s3_parser
    (const http::request_parser<http::empty_body>& recv_req,
     rest::utils::ts_unordered_map<std::string, std::shared_ptr<rest::http::model::multi_part_container>>& uomap)
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

        switch (method)
        {
            case boost::beast::http::verb::post:
                if (target.ends_with("?uploads"))
                {
                    // mechanism for creating upload id, does this mechanism create same upload id for same POST request occurring twice?
                    auto upload_id = "first_upload";

                    m_uomap_multipart.ts_insert(upload_id, std::make_shared<rest::http::model::multi_part_container>(upload_id));

//                    return std::make_unique<rest::http::model::init_multi_part_upload>(m_recv_req);
                }
                else if (target.find("partNumber=") && target.find("uploadId="))
                {
                    auto upload_id = std::string(target.substr(target.find("uploadId=") + 9, target.find("&partNumber=") - target.find("uploadId=") - 9 ));
                    auto part_number = std::stoi(std::string(target.substr(target.find("partNumber=") + 11)));

                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }
            case boost::beast::http::verb::put:
                if (!target.empty() && (target.find('?') == std::string::npos))
                {
                    return std::make_unique<rest::http::model::put_object>(m_recv_req);
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }
            default:
                throw std::runtime_error("bad http verb.");
        }

    }

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser
