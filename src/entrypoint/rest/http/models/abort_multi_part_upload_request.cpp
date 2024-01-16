#include "abort_multi_part_upload_request.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    abort_multi_part_upload_request::abort_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                     utils::server_state& server_state,
                                                                     std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_server_state(server_state),
            m_upload_id(m_uri->get_query_parameters().at("uploadId")),
            m_object_name(m_uri->get_object_key()),
            m_bucket_name(m_uri->get_bucket_id())

    {
        /*
         * grab a hold of the parts container so that we don't get segmentation fault if
         * the given upload is aborted or completed
        */
        if (!m_server_state.m_uploads.contains_upload(m_upload_id))
        {
            throw custom_error_response_exception(http::status::not_found, error::type::no_such_upload);
        }
        else
        {
            // by this time another request might have removed the upload_id
            m_server_state.m_uploads.remove_upload(m_upload_id, m_bucket_name, m_object_name);
        }
    }

    std::map<std::string, std::string> abort_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

} // uh::cluster::rest::http::model
