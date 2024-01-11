#include "list_buckets_request.h"

namespace uh::cluster::rest::http::model
{

    list_buckets_request::list_buckets_request(const http::request_parser<http::empty_body>& recv_req,
                                               std::unique_ptr<rest::http::URI> uri) : http_request(recv_req, std::move(uri))
    {
        *this = recv_req;
    }

    list_buckets_request& list_buckets_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        recv_req.get();
        return *this;
    }

    std::map<std::string, std::string> list_buckets_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;

        return headers;
    }

} // uh::cluster::http::rest::model
