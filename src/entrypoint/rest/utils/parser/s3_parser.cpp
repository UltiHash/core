#include "s3_parser.h"
#include "entrypoint/rest/http/http_types.h"
#include "entrypoint/rest/http/models/abort_multi_part_upload_request.h"
#include "entrypoint/rest/http/models/complete_multi_part_upload_request.h"
#include "entrypoint/rest/http/models/custom_error_response_exception.h"
#include "entrypoint/rest/http/models/list_multi_part_uploads_request.h"
#include "entrypoint/rest/http/models/list_objects_request.h"
#include "entrypoint/rest/http/models/multi_part_upload_request.h"
#include <iostream>

namespace uh::cluster::rest::utils::parser {

namespace model = uh::cluster::rest::http::model;

//------------------------------------------------------------------------------

s3_parser::s3_parser(const b_http::request_parser<b_http::empty_body>& recv_req,
                     utils::server_state& server_state)
    : m_recv_req(recv_req), m_server_state(server_state) {}

std::unique_ptr<rest::http::http_request> s3_parser::parse() const {
    // parse the URI
    std::unique_ptr<rest::http::URI> uri =
        std::make_unique<rest::http::URI>(m_recv_req);

    switch (uri->get_http_method()) {
    case http::http_method::HTTP_POST:
        if (!uri->get_bucket_id().empty() && !uri->get_object_key().empty()) {
            if (uri->query_string_exists("uploads")) {
                return nullptr;
            } else if (uri->query_string_exists("uploadId")) {
                // upload id should not be empty
                if (uri->get_query_parameters().at("uploadId").empty()) {
                    throw model::custom_error_response_exception(
                        b_http::status::bad_request,
                        model::error::type::bad_upload_id);
                }

                return std::make_unique<
                    rest::http::model::complete_multi_part_upload_request>(
                    m_recv_req, m_server_state, std::move(uri));
            }
        } else if (!uri->get_bucket_id().empty() &&
                   uri->get_object_key().empty()) {
            if (uri->query_string_exists("delete")) {
                return nullptr;
            }
        } else {
            throw std::runtime_error("unknown request type");
        }
    case http::http_method::HTTP_PUT:
        if (!uri->get_bucket_id().empty() && !uri->get_object_key().empty()) {
            if (uri->get_query_parameters().empty()) {
                return nullptr;
            } else if (uri->query_string_exists("partNumber") &&
                       uri->query_string_exists("uploadId")) {
                auto upload_id = uri->get_query_parameters().at("uploadId");
                if (upload_id.empty()) {
                    throw model::custom_error_response_exception(
                        b_http::status::bad_request,
                        model::error::type::bad_upload_id);
                }

                auto part_num =
                    std::stoi(uri->get_query_parameters().at("partNumber"));
                if (part_num < 1 || part_num > 10000) {
                    throw model::custom_error_response_exception(
                        b_http::status::bad_request,
                        model::error::type::bad_part_number);
                }

                return std::make_unique<
                    rest::http::model::multi_part_upload_request>(
                    m_recv_req, std::move(uri));
            }
        } else if (!uri->get_bucket_id().empty() &&
                   uri->get_object_key().empty()) {
            if (uri->get_query_parameters().empty()) {
                return nullptr;
            }
        } else {
            throw std::runtime_error("unknown request type");
        }
    case http::http_method::HTTP_GET:
        if (!uri->get_bucket_id().empty() && !uri->get_object_key().empty()) {
            if (uri->query_string_exists("attributes")) {
                return nullptr;
            } else {
                return nullptr;
            }
        } else if (!uri->get_bucket_id().empty() &&
                   uri->get_object_key().empty()) {
            if (uri->query_string_exists("uploads")) {
                return std::make_unique<
                    rest::http::model::list_multi_part_uploads_request>(
                    m_recv_req, std::move(uri));
            } else if (uri->query_string_exists("list-type") &&
                       uri->get_query_string_value("list-type") == "2") {
                return nullptr;
            } else if (uri->get_query_parameters().empty()) {
                return nullptr;
            } else // TODO: there is conflict between get_bucket_request and
                   // list_objects_request if no query string is given
            {
                return std::make_unique<
                    rest::http::model::list_objects_request>(m_recv_req,
                                                             std::move(uri));
            }
        } else if (uri->get_bucket_id().empty() &&
                   uri->get_object_key().empty()) {
            return nullptr;
        } else {
            throw std::runtime_error("unknown request type");
        }

    case http::http_method::HTTP_DELETE:
        if (!uri->get_bucket_id().empty() && !uri->get_object_key().empty()) {
            if (uri->query_string_exists("uploadId")) {
                // upload id should not be empty
                if (uri->get_query_parameters().at("uploadId").empty()) {
                    throw model::custom_error_response_exception(
                        b_http::status::bad_request,
                        model::error::type::bad_upload_id);
                }

                return std::make_unique<
                    rest::http::model::abort_multi_part_upload_request>(
                    m_recv_req, m_server_state, std::move(uri));
            } else {
                return nullptr;
            }
        } else if (!uri->get_bucket_id().empty() &&
                   uri->get_object_key().empty()) {
            if (uri->get_query_parameters().empty()) {
                return nullptr;
            }
        } else {
            throw std::runtime_error("unknown request type");
        }

    default:
        throw std::runtime_error("bad http method.");
    }
}

//------------------------------------------------------------------------------

} // namespace uh::cluster::rest::utils::parser
