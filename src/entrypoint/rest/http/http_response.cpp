#include "http_response.h"

namespace uh::cluster::rest::http
{

    http_response::http_response(const http_request& orig_req) :
    m_orig_req(orig_req),
    m_res(boost::beast::http::response<boost::beast::http::string_body>{boost::beast::http::status::ok, 11})
    {}

    http_response::http_response(const http_request& req, http::response<http::string_body> res) :
    m_orig_req(req), m_res(std::move(res))
    {}

    void http_response::set_body(std::string&& body)
    {
        m_res.body() = std::move(body);
    }

    void http_response::set_etag(const std::string& etag)
    {
        m_etag = etag;
        m_etagHasBeenSet = true;
    }

} // namespace uh::cluster::rest::http
