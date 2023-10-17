#include "abort_multi_part_upload.h"

namespace uh::cluster::rest::http::model
{

    abort_multi_part_upload::abort_multi_part_upload(const http::request_parser<http::empty_body>& recv_req,
                                                     rest::utils::ts_unordered_map<std::string, std::shared_ptr<utils::ts_map<uint16_t, std::string>>>& container,
                                                     std::string upload_id) :
            rest::http::http_request(recv_req),
            m_uomap_multipart(container),
            m_upload_id(std::move(upload_id))
    {
        m_uomap_multipart.remove(m_upload_id);
    }

    std::map<std::string, std::string> abort_multi_part_upload::get_request_specific_headers() const
    {
        return {};
    }

} // uh::cluster::rest::http::model
