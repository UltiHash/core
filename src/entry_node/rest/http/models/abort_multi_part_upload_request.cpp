#include "abort_multi_part_upload_request.h"
#include "custom_error_response_exception.h"

namespace uh::cluster::rest::http::model
{

    abort_multi_part_upload_request::abort_multi_part_upload_request(const http::request_parser<http::empty_body>& recv_req,
                                                                     rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::pair<std::string, std::string>>>>& container,
                                                                     std::string upload_id,
                                                                     std::unique_ptr<rest::http::URI> uri) :
            rest::http::http_request(recv_req, std::move(uri)),
            m_uomap_multipart(container),
            m_upload_id(std::move(upload_id))
    {
        // upload id should exist to delete it
        auto iterator = m_uomap_multipart.find(m_upload_id);
        if (iterator == m_uomap_multipart.end())
        {
            std::string m_body = "<Error>\n"
                                 "<Code>NoSuchUpload</Code>\n"
                                 "<Message>Upload id not found.</Message>\n"
                                 "</Error>";

            throw custom_error_response_exception(http::status::not_found, error::type::no_such_upload);
        }
        else
        {
            m_uomap_multipart.remove(m_upload_id);
        }
    }

    std::map<std::string, std::string> abort_multi_part_upload_request::get_request_specific_headers() const
    {
        return {};
    }

} // uh::cluster::rest::http::model
