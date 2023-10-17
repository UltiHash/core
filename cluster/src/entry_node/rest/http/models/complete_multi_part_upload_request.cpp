#include "complete_multi_part_upload_request.h"
#include <iostream>

namespace uh::cluster::rest::http::model
{

    complete_multi_part_upload_request::complete_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                           rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::string>>>& uo_container,
                                                                           std::string upload_id) :
            rest::http::http_request(recv_req),
            m_uomap_multipart(uo_container),
            m_upload_id(std::move(upload_id))
    {
    }

    complete_multi_part_upload_request::~complete_multi_part_upload_request()
    {
        clear_body();
    }

    std::map<std::string, std::string> complete_multi_part_upload_request::get_request_specific_headers() const
    {
    }

    const std::string& complete_multi_part_upload_request::get_body()
    {
        if (m_completed_body.empty())
        {
            auto iterator = m_uomap_multipart.find(m_upload_id);
            if (iterator == m_uomap_multipart.end())
                throw std::runtime_error("Invalid Upload ID");

            for (const auto& part : *iterator->second)
                m_completed_body += part.second;
        }

        return m_completed_body;
    }

    std::size_t complete_multi_part_upload_request::get_body_size() const
    {
        auto iterator = m_uomap_multipart.find(m_upload_id);
        if (iterator == m_uomap_multipart.end())
            throw std::runtime_error("Invalid Upload ID");

        size_t body_size {};
        for (const auto& part : *iterator->second)
            body_size += part.second.length();

        return body_size;
    }

    void complete_multi_part_upload_request::clear_body()
    {
        m_uomap_multipart.remove(m_upload_id);
    }

} // uh::cluster::rest::http::model
