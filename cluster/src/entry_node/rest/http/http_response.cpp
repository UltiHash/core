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

    void http_response::set_body(std::string body)
    {
        m_res.body() = std::move(body);
    }

    void http_response::set_error_body(std::string body)
    {
        m_error.body() = std::move(body);
        m_errorHasBeenSet = true;
    }

    void http_response::set_error(http::response<http::string_body> err_res)
    {
        m_errorHasBeenSet = true;
        m_error = std::move(err_res);
    }

} // namespace uh::cluster::rest::http
