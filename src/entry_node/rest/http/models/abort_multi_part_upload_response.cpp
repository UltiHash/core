#include "abort_multi_part_upload_response.h"

namespace uh::cluster::rest::http::model
{

    abort_multi_part_upload_response::abort_multi_part_upload_response(const http_request& req) :
    http_response(req, boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::no_content, 11})
    {
    }

    abort_multi_part_upload_response::abort_multi_part_upload_response(const http_request& req, http::response<http::string_body> recv_res) :
            http_response(req, std::move(recv_res))
    {}

    const http::response<http::string_body>& abort_multi_part_upload_response::get_response_specific_object()
    {

        if(m_errorHasBeenSet)
        {
            m_error.prepare_payload();
            return m_error;
        }

        if (m_requestChargedHasBeenSet)
        {
            m_res.set("x-amz-request-charged",m_requestCharged);
        }

        if (m_requestIdHasBeenSet)
        {
            m_res.set("x-amz-request-id",m_requestId);
        }

        // xml body

        m_res.prepare_payload();
        return m_res;

    }

} // namespace uh::cluster::rest::http::model
