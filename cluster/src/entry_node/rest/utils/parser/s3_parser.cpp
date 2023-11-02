#include <iostream>
#include "s3_parser.h"
#include <entry_node/rest/http/models/put_object_request.h>
#include <entry_node/rest/http/models/get_object_request.h>
#include <entry_node/rest/http/models/create_bucket_request.h>
#include <entry_node/rest/http/models/list_buckets_request.h>
#include <entry_node/rest/http/models/init_multi_part_upload_request.h>
#include <entry_node/rest/http/models/multi_part_upload_request.h>
#include <entry_node/rest/http/models/complete_multi_part_upload_request.h>
#include <entry_node/rest/http/models/abort_multi_part_upload_request.h>
#include <entry_node/rest/http/models/delete_bucket_request.h>
#include <entry_node/rest/http/models/delete_objects_request.h>
#include <entry_node/rest/http/models/delete_object_request.h>
#include <entry_node/rest/http/models/get_object_attributes_request.h>
#include <entry_node/rest/http/models/list_objectsv2_request.h>
#include <entry_node/rest/http/models/list_objects_request.h>
#include <entry_node/rest/http/models/list_multi_part_uploads_request.h>
#include <entry_node/rest/http/models/get_bucket_request.h>
#include <entry_node/rest/utils/generator/generator.h>
#include "entry_node/rest/http/http_types.h"

namespace uh::cluster::rest::utils::parser {

//------------------------------------------------------------------------------

    s3_parser::s3_parser
            (const b_http::request_parser<b_http::empty_body>& recv_req,
             rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::string>>>& uomap)
            : m_recv_req(recv_req), m_uomap_multipart(uomap)
    {}

    std::unique_ptr<rest::http::http_request>
    s3_parser::parse() const
    {
        // parse the URI
        std::unique_ptr<rest::http::URI> uri = std::make_unique<rest::http::URI>(m_recv_req);

        switch (uri->get_http_method())
        {
            case http::http_method::HTTP_POST:
                if (!uri->get_bucket_id().empty() && !uri->get_object_key().empty())
                {
                    if (uri->query_string_exists("uploads"))
                    {
                        auto upload_id = generator::generate_unique_id();
                        m_uomap_multipart.emplace(upload_id, std::make_shared<utils::ts_map<uint16_t, std::string>>());
                        return std::make_unique<rest::http::model::init_multi_part_upload_request>(m_recv_req, upload_id, std::move(uri));
                    }
                    else if (uri->query_string_exists("uploadId"))
                    {
                        auto upload_id = uri->get_query_string_value("uploadId");
                        if (upload_id.empty())
                            throw std::runtime_error("no upload id given");
                        return std::make_unique<rest::http::model::complete_multi_part_upload_request>(m_recv_req, m_uomap_multipart, upload_id,  std::move(uri));
                    }
                }
                else if (!uri->get_bucket_id().empty() && uri->get_object_key().empty())
                {
                    if (uri->query_string_exists("delete"))
                    {
                        return std::make_unique<rest::http::model::delete_objects_request>(m_recv_req, std::move(uri));
                    }
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }
            case http::http_method::HTTP_PUT:
                if (!uri->get_bucket_id().empty() && !uri->get_object_key().empty())
                {
                    if (uri->get_query_parameters().empty())
                    {
                        return std::make_unique<rest::http::model::put_object_request>(m_recv_req,  std::move(uri));
                    }
                    else if (uri->query_string_exists("partNumber") && uri->query_string_exists("uploadId"))
                    {
                        auto upload_id = uri->get_query_string_value("uploadId");
                        if (upload_id.empty())
                            throw std::runtime_error("unknown upload id");

                        auto part_string = uri->get_query_string_value("partNumber");
                        if (part_string.empty())
                            throw std::runtime_error("unknown upload id");

                        auto part_number = std::stoi(part_string);

                        auto iterator = m_uomap_multipart.find(upload_id);
                        if (iterator == m_uomap_multipart.end())
                            throw std::runtime_error("Invalid Upload ID");

                        return std::make_unique<rest::http::model::multi_part_upload_request>(m_recv_req, *iterator->second, part_number, std::move(uri));
                    }
                }
                else if (!uri->get_bucket_id().empty() && uri->get_object_key().empty())
                {
                    if (uri->get_query_parameters().empty())
                    {
                        return std::make_unique<rest::http::model::create_bucket_request>(m_recv_req, std::move(uri));
                    }
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }
            case http::http_method::HTTP_GET:
                if (!uri->get_bucket_id().empty() && !uri->get_object_key().empty())
                {
                    if (uri->query_string_exists("attributes"))
                    {
                        return std::make_unique<rest::http::model::get_object_attributes_request>(m_recv_req, std::move(uri));
                    }
                    else
                    {
                        return std::make_unique<rest::http::model::get_object_request>(m_recv_req, std::move(uri));
                    }
                }
                else if (!uri->get_bucket_id().empty() && uri->get_object_key().empty())
                {
                    if (uri->query_string_exists("uploads"))
                    {
                        return std::make_unique<rest::http::model::list_multi_part_uploads_request>(m_recv_req, std::move(uri));
                    }
                    else if (uri->query_string_exists("list-type") && uri->get_query_string_value("list-type") == "2")
                    {
                        return std::make_unique<rest::http::model::list_objectsv2_request>(m_recv_req, std::move(uri));
                    }
                    else if (uri->get_query_parameters().empty())
                    {
                        return std::make_unique<rest::http::model::get_bucket_request>(m_recv_req, std::move(uri));
                    }
                    else // TODO: there is conflict between get_bucket_request and list_objects_request if no query string is given
                    {
                        return std::make_unique<rest::http::model::list_objects_request>(m_recv_req, std::move(uri));
                    }
                }
                else if (uri->get_bucket_id().empty() && uri->get_object_key().empty())
                {
                    return std::make_unique<rest::http::model::list_buckets_request>(m_recv_req, std::move(uri));
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }

            case http::http_method::HTTP_DELETE:
                if (!uri->get_bucket_id().empty() && !uri->get_object_key().empty())
                {
                    if (uri->query_string_exists("uploadId"))
                    {
                        auto upload_id = uri->get_query_string_value("uploadId");
                        if (upload_id.empty())
                            throw std::runtime_error("No upload ID given!");

                        return std::make_unique<rest::http::model::abort_multi_part_upload_request>(m_recv_req, m_uomap_multipart, upload_id, std::move(uri));
                    }
                    else
                    {
                        return std::make_unique<rest::http::model::delete_object_request>(m_recv_req, std::move(uri));
                    }
                }
                else if (!uri->get_bucket_id().empty() && uri->get_object_key().empty())
                {
                    if (uri->get_query_parameters().empty() )
                    {
                        return std::make_unique<rest::http::model::delete_bucket_request>(m_recv_req, std::move(uri));
                    }
                }
                else
                {
                    throw std::runtime_error("unknown request type");
                }

            default:
                throw std::runtime_error("bad http method.");
        }

    }

//------------------------------------------------------------------------------

} // uh::cluster::rest::utils::parser
