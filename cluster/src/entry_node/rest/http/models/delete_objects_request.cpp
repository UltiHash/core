#include "delete_objects_request.h"

namespace uh::cluster::rest::http::model
{

    delete_objects_request::delete_objects_request(const http::request_parser<http::empty_body>& recv_req) : http_request(recv_req)
    {
    }

    std::map<std::string, std::string> delete_objects_request::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;

        return headers;
    }

} // uh::cluster::http::rest::model
