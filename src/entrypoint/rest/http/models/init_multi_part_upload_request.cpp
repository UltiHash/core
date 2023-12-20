#include "init_multi_part_upload_request.h"
#include "custom_error_response_exception.h"

// UTILS
#include "entrypoint/rest/utils/generator/generator.h"

namespace uh::cluster::rest::http::model
{

    init_multi_part_upload_request::init_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                   utils::server_state& server_state, std::unique_ptr<URI> uri) :
                                                                   rest::http::http_request(recv_req, std::move(uri)),
                                                                   m_bucket_name(m_uri->get_bucket_id()),
                                                                   m_object_name(m_uri->get_object_key())
    {
        // generate upload id and insert it to the server state
        auto upload_id = utils::generator::generate_unique_id();

        bool inserted = server_state.m_uploads.insert_upload(upload_id, m_bucket_name, m_object_name);
        if (!inserted)
        {
            throw custom_error_response_exception(http::status::internal_server_error);
        }

        // TODO remove this hack
        m_etag = std::move(upload_id);
    }

    std::map<std::string, std::string> init_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

} // uh::cluster::rest::http::model
