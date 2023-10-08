#include "create_bucket.h"

namespace uh::cluster::rest::http::model
{

    create_bucket::create_bucket(const http::request_parser<http::empty_body> & recv_req) : http_request(recv_req)
    {
        // parse and set the received request parameters
        *this = recv_req;
    }

    create_bucket& create_bucket::operator = (const http::request_parser<http::empty_body>& recv_req)
    {
        const auto& header_list = recv_req.get();

        return *this;
    }

    std::map<std::string, std::string> create_bucket::get_request_specific_headers() const
    {
        std::map<std::string, std::string> headers;
        std::stringstream ss;


        return headers;
    }

} // uh::cluster::http::rest::model
