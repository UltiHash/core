#include "complete_multi_part_upload.h"
#include <iostream>

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

    std::string complete_multi_part_upload::get_body() const
    {
        std::string body {};
        for (const auto& part : m_mpcontainer)
            body += part.second;

        return body;
    }

    std::size_t complete_multi_part_upload::get_body_size() const
    {
        size_t body_size {};
        for (const auto& part : m_mpcontainer)
            body_size += part.second.length();

        return body_size;
    }

} // uh::cluster::rest::http::model
