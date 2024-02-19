#include "s3_parser.h"
#include "entrypoint/rest/http/http_types.h"
#include "entrypoint/rest/http/models/init_multi_part_upload_request.h"
#include "entrypoint/rest/http/models/list_multi_part_uploads_request.h"
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
                return std::make_unique<
                    rest::http::model::init_multi_part_upload_request>(
                    m_recv_req, m_server_state, std::move(uri));
            }
        } else {
            throw std::runtime_error("unknown request type");
        }
    case http::http_method::HTTP_GET:
        if (!uri->get_bucket_id().empty() && uri->get_object_key().empty()) {
            if (uri->query_string_exists("uploads")) {
                return std::make_unique<
                    rest::http::model::list_multi_part_uploads_request>(
                    m_recv_req, std::move(uri));
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
