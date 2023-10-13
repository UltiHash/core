#include "list_buckets_request.h"

namespace uh::cluster::rest::http::model
{

    list_buckets_request::list_buckets_request(const http::request_parser<http::empty_body>& recv_req) : http_request(recv_req)
    {
        *this = recv_req;
    }

    list_buckets_request& list_buckets_request::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        return *this;
    }

    std::map<std::string, std::string> list_buckets_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;

        return headers;
    }

} // uh::cluster::http::rest::model
