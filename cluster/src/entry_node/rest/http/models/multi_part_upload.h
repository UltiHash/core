#pragma once

#include <entry_node/rest/http/http_request.h>
#include "entry_node/rest/utils/containers/ts_map.h"
#include <map>

namespace uh::cluster::rest::http::model
{

    class multi_part_upload : public rest::http::http_request
    {
    public:
        explicit multi_part_upload(const http::request_parser<http::empty_body>&);

        ~multi_part_upload() override = default;

        [[nodiscard]] const char * get_request_name() const override;

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:
        const http::request_parser<http::empty_body>& m_recv_req;
        std::string m_upload_id;
        utils::ts_map<uint16_t, std::string> m_parts_container;
    };

} // uh::cluster::rest::http::model