#include "http_response.h"

namespace uh::cluster::rest::http
{

    http_response::http_response(const http_request& orig_req) : m_orig_req(orig_req)
    {}



} // namespace uh::cluster::rest::http
