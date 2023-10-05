#pragma once

#include <entry_node/rest/http/http_request.h>

namespace uh::cluster::rest::http::model
{

    class abort_multi_part_upload : public rest::http::http_request
    {
    public:
        explicit abort_multi_part_upload(const http::request_parser<http::empty_body>&);

        ~abort_multi_part_upload() override = default;

        [[nodiscard]] inline const char * get_request_name() const override { return "AbortMultipartUpload"; }

        [[nodiscard]] std::map<std::string, std::string> get_request_specific_headers() const override;

    private:
        const http::request_parser<http::empty_body>& m_recv_req;
    };

} // uh::cluster::rest::http::model
