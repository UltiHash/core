#include "init_multi_part_upload_request.h"

namespace uh::cluster::rest::http::model
{

    init_multi_part_upload_request::init_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                   std::string upload_id) :
            rest::http::http_request(recv_req)
    {
        m_etag = std::move(upload_id);
    }

    std::map<std::string, std::string> init_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

} // uh::cluster::rest::http::model
