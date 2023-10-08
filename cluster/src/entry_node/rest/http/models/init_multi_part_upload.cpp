#include "init_multi_part_upload.h"

namespace uh::cluster::rest::http::model
{

    init_multi_part_upload::init_multi_part_upload(const http::request_parser<http::empty_body>& recv_req) :
            rest::http::http_request(recv_req)
    {}

    std::map<std::string, std::string> init_multi_part_upload::get_request_specific_headers() const
    {

    }

} // uh::cluster::rest::http::model
