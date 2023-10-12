#include "create_bucket_response.h"

namespace uh::cluster::rest::http::model
{

    create_object_response::create_object_response(const http_request& req) : http_response(req)
    {}

    create_object_response::create_object_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {}

    const http::response<http::string_body>& create_object_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if (m_locationHasBeenSet)
        {
            m_res.set(boost::beast::http::field::location, m_location);
        }

        if (m_requestIdHasBeenSet)
        {
            m_res.set("x-amz-request-id", m_requestId);
        }

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
