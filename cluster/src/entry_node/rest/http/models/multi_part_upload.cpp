#include "multi_part_upload.h"

namespace uh::cluster::rest::http::model
{

    multi_part_upload::multi_part_upload(const http::request_parser<http::empty_body>& recv_req) :
            m_recv_req(recv_req),
            rest::http::http_request(recv_req)
    {}

    std::map<std::string, std::string> multi_part_upload::get_request_specific_headers() const
    {

    }

} // uh::cluster::rest::http::model
