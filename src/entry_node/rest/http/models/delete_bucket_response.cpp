#include "delete_bucket_response.h"

namespace uh::cluster::rest::http::model
{

    delete_bucket_response::delete_bucket_response(const http_request& req) : http_response(req)
    {}

    delete_bucket_response::delete_bucket_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {}

    const http::response<http::string_body>& delete_bucket_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
