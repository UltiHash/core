#include "http_response.h"

namespace uh::cluster::rest::http
{

    http_response::http_response(const http_request& orig_req) : m_orig_req(orig_req)
    {
        m_res.set(http::field::server, "UltiHash v0.2.0");
        m_res.set(http::field::content_type, "plain/text");
    }

} // namespace uh::cluster::rest::http
