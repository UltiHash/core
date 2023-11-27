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
        auto ptr = m_internal_server_state.get_multipart_container().get_value(m_upload_id);
        if (ptr == nullptr)
        {
            throw custom_error_response_exception(http::status::not_found, error::type::no_such_upload);
        }
        else
        {
            m_internal_server_state.get_multipart_container().remove(m_upload_id);
        }

        auto& bucket_multiparts = m_internal_server_state.get_bucket_multiparts();
        auto keys_map_ptr = bucket_multiparts.get_value(m_bucket_name);
        if (keys_map_ptr == nullptr)
        {
            throw custom_error_response_exception(http::status::not_found, error::type::bucket_not_found);
        }

        auto vector_ptr = keys_map_ptr->get_value(m_object_name);
        if (vector_ptr == nullptr)
        {
            throw custom_error_response_exception(http::status::not_found, error::type::object_not_found);
        }


        vector_ptr->remove(m_upload_id);
        // if there are no upload ids in the object map , remove the object map
        if (vector_ptr->is_empty())
            keys_map_ptr->remove(m_object_name);

        // if there are no objects in bucket remove the whole bucket
        if (keys_map_ptr->is_empty())
        {
            bucket_multiparts.remove(m_bucket_name);
        }

        server_state.get_mp_address_container().remove(m_upload_id);

    }

    std::map<std::string, std::string> abort_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

} // uh::cluster::rest::http::model
