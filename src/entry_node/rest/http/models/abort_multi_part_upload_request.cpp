#include "abort_multi_part_upload_request.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    abort_multi_part_upload_request::abort_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                     utils::state& server_state,
                                                                     std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_internal_server_state(server_state),
            m_upload_id(m_uri->get_query_parameters().at("uploadId"))
    {
        // upload id should exist to delete it
        auto& multipart_container = m_internal_server_state.get_multipart_container();
        auto iterator = multipart_container.find(m_upload_id);
        if (iterator == multipart_container.end())
        {
            throw custom_error_response_exception(http::status::not_found, error::type::no_such_upload);
        }
        else
        {
            multipart_container.remove(m_upload_id);
        }
    }

    std::map<std::string, std::string> abort_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

} // uh::cluster::rest::http::model
