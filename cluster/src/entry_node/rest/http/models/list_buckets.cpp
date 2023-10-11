#include "list_buckets.h"

namespace uh::cluster::rest::http::model
{

    list_buckets::list_buckets(const http::request_parser<http::empty_body>& recv_req) : http_request(recv_req)
    {
    }

    std::map<std::string, std::string> list_buckets::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;

        return headers;
    }

} // uh::cluster::http::rest::model
