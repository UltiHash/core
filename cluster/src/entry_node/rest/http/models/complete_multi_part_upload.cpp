#include "complete_multi_part_upload.h"

namespace uh::cluster::rest::http::model
{

    complete_multi_part_upload::complete_multi_part_upload(const http::request_parser<http::empty_body>& recv_req,
                                                           rest::utils::ts_map<uint16_t, std::string>& container) :
            rest::http::http_request(recv_req),
            m_mpcontainer(container)
    {}

    std::map<std::string, std::string> complete_multi_part_upload::get_request_specific_headers() const
    {

    }

} // uh::cluster::rest::http::model
