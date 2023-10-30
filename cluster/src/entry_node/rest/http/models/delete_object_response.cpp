#include "delete_object_response.h"

namespace uh::cluster::rest::http::model
{

    delete_object_response::delete_object_response(const http_request& req) : http_response(req)
    {}

    delete_object_response::delete_object_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {}

    const http::response<http::string_body>& delete_object_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if (m_deleteMarkerHasBeenSet)
        {
            m_res.set("x-amz-delete-marker", m_deleteMarker);
        }

        if (m_versionIdHasBeenSet)
        {
            m_res.set("x-amz-version-id", m_versionId);
        }

        if (m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-charged", m_requestCharged);
        }

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
