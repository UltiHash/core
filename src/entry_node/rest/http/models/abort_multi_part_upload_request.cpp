#include "abort_multi_part_upload_request.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    abort_multi_part_upload_request::abort_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                     utils::state& server_state,
                                                                     std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_internal_server_state(server_state),
            m_bucket_name(m_uri->get_bucket_id()),
            m_object_name(m_uri->get_object_key()),
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
            // removing here is of no effect and can throw
            multipart_container.remove(m_upload_id);
        }

        auto& bucket_multiparts = m_internal_server_state.get_bucket_multiparts();
        auto vector_itr = bucket_multiparts.find(m_bucket_name)->second->find(m_object_name)->second;
        vector_itr->remove(m_upload_id);
        if (vector_itr->is_empty())
            bucket_multiparts.remove(m_bucket_name);
    }

    std::map<std::string, std::string> abort_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

} // uh::cluster::rest::http::model
